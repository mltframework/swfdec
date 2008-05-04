/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <pango/pangocairo.h>

#include "swfdec_text_field_movie.h"
#include "swfdec_as_context.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_interpret.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"
#include "swfdec_sandbox.h"
#include "swfdec_text_format.h"
#include "swfdec_xml.h"

G_DEFINE_TYPE (SwfdecTextFieldMovie, swfdec_text_field_movie, SWFDEC_TYPE_ACTOR)

#define EXTRA_MARGIN 2
#define BULLET_MARGIN 36

/*** CURSOR API ***/

static gsize
swfdec_text_field_movie_get_cursor (SwfdecTextFieldMovie *text)
{
  return text->cursor_start;
}

static gboolean
swfdec_text_field_movie_has_cursor (SwfdecTextFieldMovie *text)
{
  return text->cursor_start == text->cursor_end;
}
#define swfdec_text_field_movie_has_selection(text) (!swfdec_text_field_movie_has_cursor (text))

static void
swfdec_text_field_movie_get_selection (SwfdecTextFieldMovie *text, gsize *start, gsize *end)
{
  if (start)
    *start = MIN (text->cursor_start, text->cursor_end);
  if (end)
    *end = MAX (text->cursor_start, text->cursor_end);
}

static void
swfdec_text_field_movie_set_cursor (SwfdecTextFieldMovie *text, gsize start, gsize end)
{
  g_return_if_fail (start <= text->input->len);
  g_return_if_fail (end <= text->input->len);

  if (text->cursor_start == start &&
      text->cursor_end == end)
    return;

  text->cursor_start = start;
  text->cursor_end = end;
  swfdec_movie_invalidate_last (SWFDEC_MOVIE (text));
}

/*** VFUNCS ***/

static void
swfdec_text_field_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);

  swfdec_rect_union (extents, extents, &text->extents);
}

static void
swfdec_text_field_movie_invalidate (SwfdecMovie *movie, const cairo_matrix_t *matrix, gboolean last)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecRect rect;

  rect = text->extents;

  // border is drawn partly outside the extents
  if (text->border) {
    rect.x1 += SWFDEC_TWIPS_TO_DOUBLE (1);
    rect.y1 += SWFDEC_TWIPS_TO_DOUBLE (1);
  }

  swfdec_rect_transform (&rect, &rect, matrix);
  swfdec_player_invalidate (
      SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context), &rect);
}

static void
swfdec_text_field_movie_ensure_asterisks (SwfdecTextFieldMovie *text,
    guint length)
{
  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));

  if (text->asterisks_length >= length)
    return;

  if (text->asterisks != NULL)
    g_free (text->asterisks);

  text->asterisks = g_malloc (length + 1);
  memset (text->asterisks, '*', length);
  text->asterisks[length] = 0;
  text->asterisks_length = length;
}

static void
swfdec_text_paragraph_add_attribute (SwfdecParagraph *paragraph,
    PangoAttribute *attr)
{
  paragraph->attrs = g_slist_prepend (paragraph->attrs, attr);
}

static void
swfdec_text_paragraph_add_block (SwfdecParagraph *paragraph, int index_,
    SwfdecTextFormat *format)
{
  guint i;
  SwfdecBlock *block;

  block = g_new0 (SwfdecBlock, 1);

  block->index_ = index_;

  switch (format->attr.align) {
    case SWFDEC_TEXT_ALIGN_LEFT:
      block->align = PANGO_ALIGN_LEFT;
      block->justify = FALSE;
      break;
    case SWFDEC_TEXT_ALIGN_RIGHT:
      block->align = PANGO_ALIGN_RIGHT;
      block->justify = FALSE;
      break;
    case SWFDEC_TEXT_ALIGN_CENTER:
      block->align = PANGO_ALIGN_CENTER;
      block->justify = FALSE;
      break;
    case SWFDEC_TEXT_ALIGN_JUSTIFY:
      block->align = PANGO_ALIGN_LEFT;
      block->justify = TRUE;
      break;
    default:
      g_assert_not_reached ();
  }
  block->leading = format->attr.leading * 20 * PANGO_SCALE;
  block->block_indent = format->attr.block_indent * 20;
  block->left_margin = format->attr.left_margin * 20;
  block->right_margin = format->attr.right_margin * 20;

  if (format->attr.n_tab_stops != 0) {
    block->tab_stops = pango_tab_array_new (format->attr.n_tab_stops, TRUE);
    for (i = 0; i < format->attr.n_tab_stops; i++) {
      pango_tab_array_set_tab (block->tab_stops, i, PANGO_TAB_LEFT,
	  format->attr.tab_stops[i] * 20);
    }
  } else {
    block->tab_stops = NULL;
  }

  paragraph->blocks = g_slist_prepend (paragraph->blocks, block);
}

static void
swfdec_text_field_movie_generate_paragraph (SwfdecTextFieldMovie *text,
    SwfdecParagraph *paragraph, guint start_index, guint length)
{
  SwfdecTextFormat *format, *format_prev;
  guint index_;
  GSList *iter;
  PangoAttribute *attr_bold, *attr_color, *attr_font, *attr_italic,
		 *attr_letter_spacing, *attr_size, *attr_underline;
  // TODO: kerning, display

  g_assert (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_assert (paragraph != NULL);
  g_assert (start_index + length <= text->input->len);

  paragraph->index_ = start_index;
  paragraph->length = length;
  if (text->input->str[start_index + length - 1] == '\n' ||
      text->input->str[start_index + length - 1] == '\r') {
    paragraph->newline = TRUE;
  } else {
    paragraph->newline = FALSE;
  }

  paragraph->blocks = NULL;
  paragraph->attrs = NULL;

  g_assert (text->formats != NULL);
  for (iter = text->formats; iter->next != NULL &&
      ((SwfdecFormatIndex *)(iter->next->data))->index_ <= start_index;
      iter = iter->next);

  index_ = start_index;
  format = ((SwfdecFormatIndex *)(iter->data))->format;

  // Paragraph formats
  paragraph->bullet = format->attr.bullet;
  paragraph->indent = format->attr.indent * 20 * PANGO_SCALE;

  // Add new block
  swfdec_text_paragraph_add_block (paragraph, 0, format);

  // Open attributes
  attr_bold = pango_attr_weight_new (
      (format->attr.bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL));
  attr_bold->start_index = 0;

  attr_color = pango_attr_foreground_new (SWFDEC_COLOR_R (format->attr.color) * 255,
      SWFDEC_COLOR_G (format->attr.color) * 255,
      SWFDEC_COLOR_B (format->attr.color) * 255);
  attr_color->start_index = 0;

  // FIXME: embed fonts
  attr_font = pango_attr_family_new (format->attr.font);
  attr_font->start_index = 0;

  attr_italic = pango_attr_style_new (
      (format->attr.italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
  attr_italic->start_index = 0;

  attr_letter_spacing = pango_attr_letter_spacing_new (
      format->attr.letter_spacing * 20 * PANGO_SCALE);
  attr_letter_spacing->start_index = 0;

  attr_size =
    pango_attr_size_new_absolute (MAX (format->attr.size, 1) * 20 * PANGO_SCALE);
  attr_size->start_index = 0;

  attr_underline = pango_attr_underline_new (
      (format->attr.underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));
  attr_underline->start_index = 0;

  for (iter = iter->next;
      iter != NULL &&
      ((SwfdecFormatIndex *)(iter->data))->index_ < start_index + length;
      iter = iter->next)
  {
    format_prev = format;
    index_ = ((SwfdecFormatIndex *)(iter->data))->index_;
    format = ((SwfdecFormatIndex *)(iter->data))->format;

    // Add new block if necessary
    if (format_prev->attr.align != format->attr.align ||
       format_prev->attr.bullet != format->attr.bullet ||
       format_prev->attr.indent != format->attr.indent ||
       format_prev->attr.leading != format->attr.leading ||
       format_prev->attr.block_indent != format->attr.block_indent ||
       format_prev->attr.left_margin != format->attr.left_margin)
    {
      swfdec_text_paragraph_add_block (paragraph, index_ - start_index,
	  format);
    }

    // Change attributes if necessary
    if (format_prev->attr.bold != format->attr.bold) {
      attr_bold->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_bold);

      attr_bold = pango_attr_weight_new (
	  (format->attr.bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL));
      attr_bold->start_index = index_ - start_index;
    }

    if (format_prev->attr.color != format->attr.color) {
      attr_color->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_color);

      attr_color = pango_attr_foreground_new (
	  SWFDEC_COLOR_R (format->attr.color) * 255,
	  SWFDEC_COLOR_G (format->attr.color) * 255,
	  SWFDEC_COLOR_B (format->attr.color) * 255);
      attr_color->start_index = index_ - start_index;
    }

    if (format_prev->attr.font != format->attr.font) {
      attr_font->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_font);

      // FIXME: embed fonts
      attr_font = pango_attr_family_new (format->attr.font);
      attr_font->start_index = index_ - start_index;
    }

    if (format_prev->attr.italic != format->attr.italic) {
      attr_italic->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_italic);

      attr_italic = pango_attr_style_new (
	  (format->attr.italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
      attr_italic->start_index = index_ - start_index;
    }

    if (format_prev->attr.letter_spacing != format->attr.letter_spacing) {
      attr_letter_spacing->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_letter_spacing);

      attr_letter_spacing = pango_attr_letter_spacing_new (
	  format->attr.letter_spacing * 20 * PANGO_SCALE);
      attr_letter_spacing->start_index = index_ - start_index;
    }

    if (format_prev->attr.size != format->attr.size) {
      attr_size->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_size);

      attr_size = pango_attr_size_new_absolute (
	  MAX (1, format->attr.size) * 20 * PANGO_SCALE);
      attr_size->start_index = index_ - start_index;
    }

    if (format_prev->attr.underline != format->attr.underline) {
      attr_underline->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_underline);

      attr_underline = pango_attr_underline_new (
	  (format->attr.underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));
      attr_underline->start_index = index_ - start_index;
    }
  }

  // Close attributes
  attr_bold->end_index = length;
  swfdec_text_paragraph_add_attribute (paragraph, attr_bold);
  attr_bold = NULL;

  attr_color->end_index = length;
  swfdec_text_paragraph_add_attribute (paragraph, attr_color);
  attr_color = NULL;

  attr_font->end_index = length;
  swfdec_text_paragraph_add_attribute (paragraph, attr_font);
  attr_font = NULL;

  attr_italic->end_index = length;
  swfdec_text_paragraph_add_attribute (paragraph, attr_italic);
  attr_italic = NULL;

  attr_letter_spacing->end_index = length;
  swfdec_text_paragraph_add_attribute (paragraph, attr_letter_spacing);
  attr_letter_spacing = NULL;

  attr_size->end_index = length;
  swfdec_text_paragraph_add_attribute (paragraph, attr_size);
  attr_size = NULL;

  attr_underline->end_index = length;
  swfdec_text_paragraph_add_attribute (paragraph, attr_underline);
  attr_underline = NULL;

  // reverse blocks since we use prepend to add them
  paragraph->blocks = g_slist_reverse (paragraph->blocks);
}

static SwfdecParagraph *
swfdec_text_field_movie_get_paragraphs (SwfdecTextFieldMovie *text, int *num)
{
  GArray *paragraphs;
  SwfdecParagraph paragraph;
  const char *p, *end;
  guint max_length;

  g_assert (SWFDEC_IS_TEXT_FIELD_MOVIE (text));

  paragraphs = g_array_new (TRUE, TRUE, sizeof (SwfdecParagraph));

  max_length = 0;
  p = text->input->str;
  while (*p != '\0')
  {
    end = strpbrk (p, "\r\n");
    if (end == NULL) {
      end = strchr (p, '\0');
    } else {
      end++;
    }

    if ((guint) (end - p) > max_length)
      max_length = end - p;

    swfdec_text_field_movie_generate_paragraph (text, &paragraph,
	p - text->input->str, end - p);
    paragraphs = g_array_append_val (paragraphs, paragraph);

    p = end;
  }

  if (num != NULL)
    *num = paragraphs->len;

  if (text->password)
    swfdec_text_field_movie_ensure_asterisks (text, max_length);

  return (SwfdecParagraph *) (void *) g_array_free (paragraphs, FALSE);
}

static void
swfdec_text_field_movie_free_paragraphs (SwfdecParagraph *paragraphs)
{
  GSList *iter;
  int i;

  g_return_if_fail (paragraphs != NULL);

  for (i = 0; paragraphs[i].blocks != NULL; i++)
  {
    for (iter = paragraphs[i].blocks; iter != NULL; iter = iter->next) {
      if (((SwfdecBlock *)(iter->data))->tab_stops)
	pango_tab_array_free (((SwfdecBlock *)(iter->data))->tab_stops);
      g_free (iter->data);
    }
    g_slist_free (paragraphs[i].blocks);

    for (iter = paragraphs[i].attrs; iter != NULL; iter = iter->next) {
      pango_attribute_destroy ((PangoAttribute *)(iter->data));
    }
    g_slist_free (paragraphs[i].attrs);
  }
  g_free (paragraphs);
}

/*
 * Rendering
 */
static PangoAttrList *
swfdec_text_field_movie_paragraph_get_attr_list (
    const SwfdecParagraph *paragraph, guint index_,
    const SwfdecColorTransform *trans)
{
  PangoAttrList *attr_list;
  GSList *iter;

  attr_list = pango_attr_list_new ();

  for (iter = paragraph->attrs; iter != NULL; iter = iter->next)
  {
    PangoAttribute *attr;

    if (((PangoAttribute *)iter->data)->end_index <= index_)
      continue;

    attr = pango_attribute_copy ((PangoAttribute *)iter->data);

    if (attr->klass->type == PANGO_ATTR_FOREGROUND && trans != NULL &&
	!swfdec_color_transform_is_identity (trans))
    {
      SwfdecColor color;
      PangoColor color_p;

      color_p = ((PangoAttrColor *)attr)->color;

      color = SWFDEC_COLOR_COMBINE (color_p.red >> 8, color_p.green >> 8,
	  color_p.blue >> 8, 255);
      color = swfdec_color_apply_transform (color, trans);

      color_p.red = SWFDEC_COLOR_R (color) << 8;
      color_p.green = SWFDEC_COLOR_G (color) << 8;
      color_p.blue = SWFDEC_COLOR_B (color) << 8;
    }

    attr->start_index =
      (attr->start_index > index_ ? attr->start_index - index_ : 0);
    attr->end_index = attr->end_index - index_;
    pango_attr_list_insert (attr_list, attr);
  }

  return attr_list;
}

static void
swfdec_text_field_movie_attr_list_get_ascent_descent (PangoAttrList *attr_list,
    guint pos, int *ascent, int *descent)
{
  PangoAttrIterator *attr_iter;
  PangoFontDescription *desc;
  PangoFontMap *fontmap;
  PangoFont *font;
  PangoFontMetrics *metrics;
  PangoContext *pcontext;
  int end;

  if (ascent != NULL)
    *ascent = 0;
  if (descent != NULL)
    *descent = 0;

  g_return_if_fail (attr_list != NULL);

  attr_iter = pango_attr_list_get_iterator (attr_list);
  pango_attr_iterator_range (attr_iter, NULL, &end);
  while ((guint)end < pos && pango_attr_iterator_next (attr_iter)) {
    pango_attr_iterator_range (attr_iter, NULL, &end);
  }
  desc = pango_font_description_new ();
  pango_attr_iterator_get_font (attr_iter, desc, NULL, NULL);
  fontmap = pango_cairo_font_map_get_default ();
  pcontext =
    pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (fontmap));
  font = pango_font_map_load_font (fontmap, pcontext, desc);
  metrics = pango_font_get_metrics (font, NULL);

  if (ascent != NULL)
    *ascent = pango_font_metrics_get_ascent (metrics) / PANGO_SCALE;
  if (descent != NULL)
    *descent = pango_font_metrics_get_descent (metrics) / PANGO_SCALE;

  g_object_unref (pcontext);
  pango_font_metrics_unref (metrics);
  pango_font_description_free (desc);
  pango_attr_iterator_destroy (attr_iter);
}

static SwfdecLayout *
swfdec_text_field_movie_get_layouts (SwfdecTextFieldMovie *text, int *num,
    cairo_t *cr, const SwfdecParagraph *paragraphs,
    const SwfdecColorTransform *trans)
{
  GArray *layouts;
  guint i;
  SwfdecParagraph *paragraphs_free;
  gsize cursor_start, cursor_end;

  g_assert (SWFDEC_IS_TEXT_FIELD_MOVIE (text));

  if (cr == NULL) {
    cr = cairo_create (swfdec_renderer_get_surface (
	  SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context)->priv->renderer));
  } else {
    cairo_reference (cr);
  }

  if (paragraphs == NULL) {
    paragraphs_free = swfdec_text_field_movie_get_paragraphs (text, NULL);
    paragraphs = paragraphs_free;
  } else {
    paragraphs_free = NULL;
  }

  layouts = g_array_new (TRUE, TRUE, sizeof (SwfdecLayout));
  swfdec_text_field_movie_get_selection (text, &cursor_start, &cursor_end);

  for (i = 0; paragraphs[i].blocks != NULL; i++)
  {
    GSList *iter;
    guint skip;

    skip = 0;
    for (iter = paragraphs[i].blocks; iter != NULL; iter = iter->next)
    {
      SwfdecLayout layout;
      PangoLayout *playout;
      PangoAttrList *attr_list;
      SwfdecBlock *block;
      int width;
      guint length;

      block = (SwfdecBlock *)iter->data;
      if (iter->next != NULL) {
	length =
	  ((SwfdecBlock *)(iter->next->data))->index_ - block->index_;
      } else {
	length = paragraphs[i].length - block->index_;
	if (paragraphs[i].newline)
	  length -= 1;
      }

      if (skip > length) {
	skip -= length;
	continue;
      }

      // create layout
      playout = layout.layout = pango_cairo_create_layout (cr);
      pango_layout_set_single_paragraph_mode (playout, TRUE);

      // set rendering position
      layout.offset_x = block->left_margin + block->block_indent;
      if (paragraphs[i].bullet)
	layout.offset_x += SWFDEC_DOUBLE_TO_TWIPS (BULLET_MARGIN);

      width = SWFDEC_MOVIE (text)->original_extents.x1 -
	SWFDEC_MOVIE (text)->original_extents.x0 - block->right_margin -
	layout.offset_x;

      if (block->index_ == 0 && paragraphs[i].indent < 0) {
	// limit negative indent to not go over leftMargin + blockIndent
	int indent = MAX (paragraphs[i].indent / PANGO_SCALE,
	    -(block->left_margin + block->block_indent));
	layout.offset_x += indent;
	width += -indent;
      }

      if (text->word_wrap) {
	pango_layout_set_wrap (playout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_width (playout, width * PANGO_SCALE);
	pango_layout_set_alignment (playout, block->align);
	pango_layout_set_justify (playout, block->justify);
      } else {
	pango_layout_set_width (playout, -1);
      }

      // set paragraph styles
      if (block->index_ == 0) {
	pango_layout_set_indent (playout, paragraphs[i].indent);
	layout.bullet = paragraphs[i].bullet;
      } else {
	pango_layout_set_indent (playout, 0);
	layout.bullet = FALSE;
      }

      // set block styles
      pango_layout_set_spacing (playout, block->leading);
      if (block->tab_stops != NULL)
	pango_layout_set_tabs (playout, block->tab_stops);

      // set text attributes
      attr_list = swfdec_text_field_movie_paragraph_get_attr_list (
	  &paragraphs[i], block->index_ + skip, trans);

      // add background for selection
      layout.index_ = paragraphs[i].index_ + block->index_ + skip;
      layout.index_end = layout.index_ + length;
      if (swfdec_text_field_movie_has_selection (text) &&
	  layout.index_ < cursor_end) {
	SwfdecColor color;
	PangoAttribute *attr_fg, *attr_bg;

	color = SWFDEC_COLOR_COMBINE (255, 255, 255, 255);
	if (trans != NULL)
	  color = swfdec_color_apply_transform (color, trans);
	attr_fg = pango_attr_foreground_new (SWFDEC_COLOR_R (color) << 8,
	    SWFDEC_COLOR_G (color) << 8, SWFDEC_COLOR_B (color) << 8);

	color = SWFDEC_COLOR_COMBINE (0, 0, 0, 255);
	if (trans != NULL)
	  color = swfdec_color_apply_transform (color, trans);
	attr_bg = pango_attr_background_new (SWFDEC_COLOR_R (color) << 8,
	    SWFDEC_COLOR_G (color) << 8, SWFDEC_COLOR_B (color) << 8);

	if (cursor_start > layout.index_) {
	  attr_fg->start_index = attr_bg->start_index = cursor_start - layout.index_;
	} else {
	  attr_fg->start_index = attr_bg->start_index = 0;
	}
	attr_bg->end_index = attr_fg->end_index = cursor_end - layout.index_;

	pango_attr_list_insert (attr_list, attr_fg);
	pango_attr_list_insert (attr_list, attr_bg);
      }

      // add shape for newline character at the end
      if (paragraphs[i].newline) {
	int ascent, descent;
	PangoRectangle rect;
	PangoAttribute *attr;

	swfdec_text_field_movie_attr_list_get_ascent_descent (attr_list,
	    paragraphs[i].length - block->index_ - skip, &ascent, &descent);

	rect.x = 0;
	rect.width = 0;
	rect.y = -ascent * PANGO_SCALE;
	rect.height = descent * PANGO_SCALE;

	attr = pango_attr_shape_new (&rect, &rect);

	attr->end_index = paragraphs[i].length - block->index_ - skip;
	attr->start_index = attr->end_index - 1;

	pango_attr_list_insert (attr_list, attr);
      }

      pango_layout_set_attributes (playout, attr_list);

      if (text->password) {
	pango_layout_set_text (playout, text->asterisks, paragraphs[i].length -
	    block->index_ - skip);
      } else {
	pango_layout_set_text (playout,
	    text->input->str + paragraphs[i].index_ + block->index_ + skip,
	    paragraphs[i].length - block->index_ - skip);
      }

      if (iter->next != NULL && text->word_wrap)
      {
	PangoLayoutLine *line;
	int line_num;
	guint skip_new;

	pango_layout_index_to_line_x (playout, length - skip, FALSE, &line_num,
	    NULL);
	if (line_num < pango_layout_get_line_count (playout) - 1) {
	  line = pango_layout_get_line_readonly (playout, line_num);
	  skip_new = line->start_index + line->length - (length - skip);
	  pango_layout_set_text (playout,
	      text->input->str + paragraphs[i].index_ + block->index_ + skip,
	      length - skip + skip_new);
	  skip = skip_new;
	}
      }
      else
      {
	if (!text->word_wrap && block->align != PANGO_ALIGN_LEFT) {
	  int line_width;
	  pango_layout_get_pixel_size (playout, &line_width, 0);
	  if (line_width < width) {
	    if (block->align == PANGO_ALIGN_RIGHT) {
	      layout.offset_x += width - line_width;
	    } else if (block->align == PANGO_ALIGN_CENTER) {
	      layout.offset_x += (width - line_width) / 2;
	    } else {
	      g_assert_not_reached ();
	    }
	  }
	}

	skip = 0;
      }

      pango_layout_get_pixel_size (playout, &layout.width, &layout.height);
      layout.width += layout.offset_x + block->right_margin;

      pango_attr_list_unref (attr_list);

      // add leading to last line too
      layout.height += block->leading / PANGO_SCALE;

      layouts = g_array_append_val (layouts, layout);

      if (!text->word_wrap)
	break;
    }
  }

  if (paragraphs_free != NULL) {
    swfdec_text_field_movie_free_paragraphs (paragraphs_free);
    paragraphs_free = NULL;
    paragraphs = NULL;
  }

  if (num != NULL)
    *num = layouts->len;
  cairo_destroy (cr);

  return (SwfdecLayout *) (void *) g_array_free (layouts, FALSE);
}

static void
swfdec_text_field_movie_free_layouts (SwfdecLayout *layouts)
{
  int i;

  g_return_if_fail (layouts != NULL);

  for (i = 0; layouts[i].layout != NULL; i++) {
    g_object_unref (layouts[i].layout);
  }

  g_free (layouts);
}

static void
swfdec_text_field_movie_line_position (SwfdecLayout *layouts, int line_num,
    int *pixels, int *layout_num, int *line_in_layout)
{
  int current, i;

  if (pixels != NULL)
    *pixels = 0;

  if (layout_num != NULL)
    *layout_num = 0;

  if (line_in_layout != NULL)
    *line_in_layout = 0;

  g_return_if_fail (layouts != NULL);

  current = 1;
  for (i = 0; layouts[i].layout != NULL && current < line_num; i++) {
    current += pango_layout_get_line_count (layouts[i].layout);

    if (pixels != NULL)
      *pixels += layouts[i].height;
  }

  if (current > line_num) {
    PangoLayoutIter *iter_line;

    i--;
    current -= pango_layout_get_line_count (layouts[i].layout);

    if (pixels != NULL)
      *pixels -= layouts[i].height;

    iter_line = pango_layout_get_iter (layouts[i].layout);

    while (++current < line_num) {
      g_assert (!pango_layout_iter_at_last_line (iter_line));
      pango_layout_iter_next_line (iter_line);

      if (line_in_layout != NULL)
	(*line_in_layout)++;
    }

    if (pixels != NULL) {
      PangoRectangle rect;

      pango_layout_iter_get_line_extents (iter_line, NULL, &rect);
      pango_extents_to_pixels (NULL, &rect);

      *pixels += rect.y;
    }

    pango_layout_iter_free (iter_line);
  }

  if (layout_num != NULL)
    *layout_num = i;
}

static gboolean
swfdec_text_field_movie_has_focus (SwfdecTextFieldMovie *text)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context);

  return swfdec_player_has_focus (player, SWFDEC_ACTOR (text));
}

static void
swfdec_text_field_movie_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  SwfdecTextFieldMovie *text;
  SwfdecLayout *layouts;
  SwfdecRect limit;
  SwfdecColor color;
  SwfdecParagraph *paragraphs;
  int i, y, x, skip;
  gboolean first;
  gsize cursor;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (movie));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (trans != NULL);
  g_return_if_fail (inval != NULL);

  /* textfields don't mask */
  if (swfdec_color_transform_is_mask (trans))
    return;

  text = SWFDEC_TEXT_FIELD_MOVIE (movie);

  paragraphs = swfdec_text_field_movie_get_paragraphs (text, NULL);

  swfdec_rect_intersect (&limit, &movie->original_extents, inval);

  if (text->background) {
    cairo_rectangle (cr, limit.x0, limit.y0, limit.x1 - limit.x0,
	limit.y1 - limit.y0);
    color = swfdec_color_apply_transform (text->background_color, trans);
    // always use full alpha
    swfdec_color_set_source (cr, color | SWFDEC_COLOR_COMBINE (0, 0, 0, 255));
    cairo_fill (cr);
  }

  if (text->border) {
    // FIXME: should not be scaled, but always be 1 pixel width
    // TODO: should clip before to make things faster?
    cairo_rectangle (cr, movie->original_extents.x0 +
	SWFDEC_DOUBLE_TO_TWIPS (0.5), movie->original_extents.y0 +
	SWFDEC_DOUBLE_TO_TWIPS (0.5),
	movie->original_extents.x1 - movie->original_extents.x0,
	movie->original_extents.y1 - movie->original_extents.y0);
    color = swfdec_color_apply_transform (text->border_color, trans);
    // always use full alpha
    swfdec_color_set_source (cr, color | SWFDEC_COLOR_COMBINE (0, 0, 0, 255));
    cairo_set_line_width (cr, SWFDEC_DOUBLE_TO_TWIPS (1));
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    cairo_stroke (cr);
  }

  cairo_rectangle (cr, limit.x0, limit.y0, limit.x1 - limit.x0,
      limit.y1 - limit.y0);
  cairo_clip (cr);

  layouts = swfdec_text_field_movie_get_layouts (text, NULL, cr,
      paragraphs, trans);

  first = TRUE;
  x = movie->original_extents.x0 + SWFDEC_DOUBLE_TO_TWIPS (EXTRA_MARGIN) +
    MIN (text->hscroll, text->hscroll_max);
  y = movie->original_extents.y0 + SWFDEC_DOUBLE_TO_TWIPS (EXTRA_MARGIN);

  swfdec_text_field_movie_line_position (layouts,
      MIN (text->scroll, text->scroll_max), NULL, &i, &skip);

  if (text->editable && swfdec_text_field_movie_has_focus (text) &&
      swfdec_text_field_movie_has_cursor (text))
    cursor = swfdec_text_field_movie_get_cursor (text);
  else
    cursor = G_MAXSIZE;

  for (; layouts[i].layout != NULL && y < limit.y1; i++)
  {
    SwfdecLayout *layout = &layouts[i];
    PangoLayoutIter *iter_line;
    PangoLayoutLine *line;
    PangoRectangle rect;
    int skipped;

    iter_line = pango_layout_get_iter (layout->layout);

    if (layout->bullet && skip == 0) {
      PangoColor color_p;
      PangoAttribute *attr;
      PangoAttrIterator *attr_iter;

      pango_layout_iter_get_line_extents (iter_line, NULL, &rect);
      pango_extents_to_pixels (NULL, &rect);

      cairo_new_sub_path (cr);

      // get current color
      attr_iter = pango_attr_list_get_iterator (
	  pango_layout_get_attributes (layout->layout));
      attr = pango_attr_iterator_get (attr_iter, PANGO_ATTR_FOREGROUND);
      color_p = ((PangoAttrColor *)attr)->color;
      color = SWFDEC_COLOR_COMBINE (color_p.red >> 8, color_p.green >> 8,
	  color_p.blue >> 8, 255);
      color = swfdec_color_apply_transform (color, trans);
      pango_attr_iterator_destroy (attr_iter);

      swfdec_color_set_source (cr, color);

      cairo_arc (cr, x + layout->offset_x +
	  pango_layout_get_indent (layout->layout) -
	  SWFDEC_DOUBLE_TO_TWIPS (BULLET_MARGIN) / 2,
	  y + rect.height / 2, rect.height / 8, 20, 2 * M_PI);
      cairo_fill (cr);
    }

    if (skip > 0) {
      for (skipped = 0; skipped < skip; skipped++) {
	g_assert (!pango_layout_iter_at_last_line (iter_line));
	pango_layout_iter_next_line (iter_line);
      }

      pango_layout_iter_get_line_extents (iter_line, NULL, &rect);
      pango_extents_to_pixels (NULL, &rect);

      skipped = rect.y;
      skip = 0;
    } else {
      skipped = 0;
    }

    do {
      pango_layout_iter_get_line_extents (iter_line, NULL, &rect);
      pango_extents_to_pixels (NULL, &rect);
      line = pango_layout_iter_get_line_readonly (iter_line);

      if (!first && y + rect.y + rect.height > movie->original_extents.y1)
	break;

      first = FALSE;

      if (y + rect.y > limit.y1)
	break;

      if (y + rect.y + rect.height < limit.y0 ||
	  x + layout->offset_x + rect.x > limit.x1 ||
	  x + layout->offset_x + rect.x + rect.width < limit.x0)
	continue;

      cairo_move_to (cr, x, y);
      cairo_rel_move_to (cr, layout->offset_x + rect.x,
	  pango_layout_iter_get_baseline (iter_line) / PANGO_SCALE - skipped);

      pango_cairo_show_layout_line (cr, line);
      line = NULL;
    } while (pango_layout_iter_next_line (iter_line));

    if (layouts[i].index_ <= cursor && 
	(layouts[i].index_end > cursor || (layouts[i].index_end == cursor && cursor == text->input->len)) &&
	(line == NULL || layouts[i].index_ + line->start_index >= cursor)) {
      SwfdecTextFormat *format = ((SwfdecFormatIndex *) g_slist_last (text->formats))->format;
      PangoRectangle cursor_rect;

      pango_layout_get_cursor_pos (layouts[i].layout, 
	  swfdec_text_field_movie_get_cursor (text) - layouts[i].index_,
	  &cursor_rect, NULL);

      cairo_save (cr);
      if (format && SWFDEC_TEXT_ATTRIBUTE_IS_SET (format->values_set, SWFDEC_TEXT_ATTRIBUTE_COLOR))
	swfdec_color_set_source (cr, format->attr.color | SWFDEC_COLOR_COMBINE (0, 0, 0, 0xFF));
      else
	swfdec_color_set_source (cr, text->format_new->attr.color | SWFDEC_COLOR_COMBINE (0, 0, 0, 0xFF));

      /* FIXME: what's the propwer line width here? */
      cairo_set_line_width (cr, SWFDEC_DOUBLE_TO_TWIPS (0.5));
      cairo_move_to (cr, x + layout->offset_x + rect.x, y - skipped);
      cairo_rel_move_to (cr, (double) cursor_rect.x / PANGO_SCALE, (double) cursor_rect.y / PANGO_SCALE);
      cairo_rel_line_to (cr, (double) cursor_rect.width / PANGO_SCALE, (double) cursor_rect.height / PANGO_SCALE);
      cairo_stroke (cr);
      cairo_restore (cr);
    }

    y += layout->height - skipped;

    pango_layout_iter_free (iter_line);
  }

  swfdec_text_field_movie_free_layouts (layouts);

  swfdec_text_field_movie_free_paragraphs (paragraphs);
}

void
swfdec_text_field_movie_update_scroll (SwfdecTextFieldMovie *text,
    gboolean check_limits)
{
  SwfdecLayout *layouts;
  int i, num, y, visible, all, height;
  double width, width_max;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));

  layouts = swfdec_text_field_movie_get_layouts (text, &num, NULL, NULL, NULL);

  width = SWFDEC_MOVIE (text)->original_extents.x1 -
    SWFDEC_MOVIE (text)->original_extents.x0;
  height = SWFDEC_MOVIE (text)->original_extents.y1 -
    SWFDEC_MOVIE (text)->original_extents.y0;

  width_max = width;
  y = 0;
  all = 0;
  visible = 0;

  for (i = num - 1; i >= 0; i--)
  {
    SwfdecLayout *layout = &layouts[i];
    PangoLayoutIter *iter_line;
    PangoRectangle rect;

    if (layouts[i].width > width_max)
      width_max = layouts[i].width;

    y += layout->height;

    iter_line = pango_layout_get_iter (layout->layout);

    do {
      pango_layout_iter_get_line_extents (iter_line, NULL, &rect);
      pango_extents_to_pixels (NULL, &rect);

      if (y - rect.y <= height)
	visible++;

      all++;
    } while (pango_layout_iter_next_line (iter_line));

    pango_layout_iter_free (iter_line);
  }

  swfdec_text_field_movie_free_layouts (layouts);
  layouts = NULL;

  if (text->scroll_max != all - visible + 1) {
    text->scroll_max = all - visible + 1;
    text->scroll_changed = TRUE;
  }
  if (text->hscroll_max != SWFDEC_TWIPS_TO_DOUBLE (width_max - width)) {
    text->hscroll_max = SWFDEC_TWIPS_TO_DOUBLE (width_max - width);
    text->scroll_changed = TRUE;
  }

  if (check_limits) {
    if (text->scroll != CLAMP(text->scroll, 1, text->scroll_max)) {
      text->scroll = CLAMP(text->scroll, 1, text->scroll_max);
      text->scroll_changed = TRUE;
    }
    if (text->scroll_bottom != text->scroll + (visible > 0 ? visible - 1 : 0))
    {
      text->scroll_bottom = text->scroll + (visible > 0 ? visible - 1 : 0);
      text->scroll_changed = TRUE;
    }
    if (text->hscroll != CLAMP(text->hscroll, 0, text->hscroll_max)) {
      text->hscroll = CLAMP(text->hscroll, 0, text->hscroll_max);
      text->scroll_changed = TRUE;
    }
  } else {
    if (text->scroll_bottom < text->scroll ||
	text->scroll_bottom > text->scroll_max + visible - 1) {
      text->scroll_bottom = text->scroll;
      text->scroll_changed = TRUE;
    }
  }
}

void
swfdec_text_field_movie_get_text_size (SwfdecTextFieldMovie *text, int *width,
    int *height)
{
  SwfdecLayout *layouts;
  int i;

  if (width != NULL)
    *width = 0;
  if (height != NULL)
    *height = 0;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (width != NULL || height != NULL);

  layouts = swfdec_text_field_movie_get_layouts (text, NULL, NULL, NULL, NULL);

  for (i = 0; layouts[i].layout != NULL; i++) {
    if (!text->word_wrap) {
      if (width != NULL && layouts[i].width > *width)
	*width = layouts[i].width;
    }

    if (height != NULL)
      *height += layouts[i].height;
  }

  // align to get integer amount after TWIPS_TO_DOUBLE
  if (width != NULL && *width % SWFDEC_TWIPS_SCALE_FACTOR != 0)
    *width += SWFDEC_TWIPS_SCALE_FACTOR - *width % SWFDEC_TWIPS_SCALE_FACTOR;
  if (height != NULL && *height % SWFDEC_TWIPS_SCALE_FACTOR != 0)
    *height += SWFDEC_TWIPS_SCALE_FACTOR - *height % SWFDEC_TWIPS_SCALE_FACTOR;

  swfdec_text_field_movie_free_layouts (layouts);
}

gboolean
swfdec_text_field_movie_auto_size (SwfdecTextFieldMovie *text)
{
  int height, width, diff;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), FALSE);

  if (text->auto_size == SWFDEC_AUTO_SIZE_NONE)
    return FALSE;

  swfdec_text_field_movie_get_text_size (text, &width, &height);
  width += SWFDEC_DOUBLE_TO_TWIPS (2 * EXTRA_MARGIN);
  height += SWFDEC_DOUBLE_TO_TWIPS (2 * EXTRA_MARGIN);

  if ((text->word_wrap ||
	text->extents.x1 - text->extents.x0 == width) &&
      text->extents.y1 - text->extents.y0 == height)
    return FALSE;

  swfdec_movie_invalidate_next (SWFDEC_MOVIE (text));

  if (!text->word_wrap && text->extents.x1 -
      text->extents.x0 != width)
  {
    switch (text->auto_size) {
      case SWFDEC_AUTO_SIZE_LEFT:
	text->extents.x1 = text->extents.x0 + width;
	break;
      case SWFDEC_AUTO_SIZE_RIGHT:
	text->extents.x0 = text->extents.x1 - width;
	break;
      case SWFDEC_AUTO_SIZE_CENTER:
	diff = (text->extents.x1 - text->extents.x0) - width;
	text->extents.x0 += floor (diff / 2.0);
	text->extents.x1 = text->extents.x0 + width;
	break;
      case SWFDEC_AUTO_SIZE_NONE:
      default:
	g_return_val_if_reached (FALSE);
    }
  }

  if (text->extents.y1 - text->extents.y0 != height)
  {
    text->extents.y1 = text->extents.y0 + height;
  }

  swfdec_movie_queue_update (SWFDEC_MOVIE (text),
      SWFDEC_MOVIE_INVALID_EXTENTS);

  return TRUE;
}

static void
swfdec_text_field_movie_dispose (GObject *object)
{
  SwfdecTextFieldMovie *text;
  GSList *iter;

  text = SWFDEC_TEXT_FIELD_MOVIE (object);

  if (text->asterisks != NULL) {
    g_free (text->asterisks);
    text->asterisks = NULL;
    text->asterisks_length = 0;
  }

  if (text->style_sheet) {
    if (SWFDEC_IS_STYLESHEET (text->style_sheet)) {
      g_signal_handlers_disconnect_matched (text->style_sheet, 
	  G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, text);
    }
    text->style_sheet = NULL;
  }

  for (iter = text->formats; iter != NULL; iter = iter->next) {
    g_free (text->formats->data);
    text->formats->data = NULL;
  }
  g_slist_free (text->formats);
  text->formats = NULL;

  g_string_free (text->input, TRUE);
  text->input = NULL;

  G_OBJECT_CLASS (swfdec_text_field_movie_parent_class)->dispose (object);
}

static void
swfdec_text_field_movie_mark (SwfdecAsObject *object)
{
  SwfdecTextFieldMovie *text;
  GSList *iter;

  text = SWFDEC_TEXT_FIELD_MOVIE (object);

  if (text->variable != NULL)
    swfdec_as_string_mark (text->variable);
  swfdec_as_object_mark (SWFDEC_AS_OBJECT (text->format_new));
  for (iter = text->formats; iter != NULL; iter = iter->next) {
    swfdec_as_object_mark (
	SWFDEC_AS_OBJECT (((SwfdecFormatIndex *)(iter->data))->format));
  }
  swfdec_as_object_mark (SWFDEC_AS_OBJECT (text->format_new));
  if (text->style_sheet != NULL)
    swfdec_as_object_mark (text->style_sheet);
  if (text->style_sheet_input != NULL)
    swfdec_as_string_mark (text->style_sheet_input);
  if (text->restrict_ != NULL)
    swfdec_as_string_mark (text->restrict_);

  SWFDEC_AS_OBJECT_CLASS (swfdec_text_field_movie_parent_class)->mark (object);
}

static void
swfdec_text_field_movie_init_movie (SwfdecMovie *movie)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecTextField *text_field = SWFDEC_TEXT_FIELD (movie->graphic);
  SwfdecAsContext *cx;
  SwfdecAsValue val;
  gboolean needs_unuse;

  needs_unuse = swfdec_sandbox_try_use (movie->resource->sandbox);

  cx = SWFDEC_AS_OBJECT (movie)->context;

  swfdec_text_field_movie_init_properties (cx);

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextField, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (movie),
	SWFDEC_AS_VALUE_GET_OBJECT (&val));
  }

  // listen self
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (movie));
  swfdec_as_object_call (SWFDEC_AS_OBJECT (movie), SWFDEC_AS_STR_addListener,
      1, &val, NULL);

  // format
  text->format_new =
    SWFDEC_TEXT_FORMAT (swfdec_text_format_new_no_properties (cx));
  if (!text->format_new)
    goto out;

  text->border_color = SWFDEC_COLOR_COMBINE (0, 0, 0, 0);
  text->background_color = SWFDEC_COLOR_COMBINE (255, 255, 255, 0);

  swfdec_text_format_set_defaults (text->format_new);
  if (text_field) {
    text->format_new->attr.color = text_field->color;
    text->format_new->attr.align = text_field->align;
    if (text_field->font != NULL)  {
      text->format_new->attr.font =
	swfdec_as_context_get_string (cx, text_field->font);
    }
    text->format_new->attr.size = text_field->size / 20;
    text->format_new->attr.left_margin = text_field->left_margin / 20;
    text->format_new->attr.right_margin = text_field->right_margin / 20;
    text->format_new->attr.indent = text_field->indent / 20;
    text->format_new->attr.leading = text_field->leading / 20;
  }

  // text
  if (text_field && text_field->input != NULL) {
    swfdec_text_field_movie_set_text (text,
	swfdec_as_context_get_string (cx, text_field->input),
	text->html);
  } else {
    swfdec_text_field_movie_set_text (text, SWFDEC_AS_STR_EMPTY,
	text->html);
  }

  // variable
  if (text_field && text_field->variable != NULL) {
    swfdec_text_field_movie_set_listen_variable (text,
	swfdec_as_context_get_string (cx, text_field->variable));
  }

out:
  if (needs_unuse)
    swfdec_sandbox_unuse (movie->resource->sandbox);
}

static void
swfdec_text_field_movie_finish_movie (SwfdecMovie *movie)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);

  swfdec_text_field_movie_set_listen_variable (text, NULL);
}

static void
swfdec_text_field_movie_iterate (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);

  while (text->changed) {
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_CHANGED);
    text->changed--;
  }
  if (text->scroll_changed) {
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_SCROLL);
    text->scroll_changed = FALSE;
  }
}

static SwfdecMovie *
swfdec_text_field_movie_contains (SwfdecMovie *movie, double x, double y,
    gboolean events)
{
  if (events) {
    /* check for movies in a higher layer that react to events */
    SwfdecMovie *ret;
    ret = SWFDEC_MOVIE_CLASS (swfdec_text_field_movie_parent_class)->contains (
	movie, x, y, TRUE);
    if (ret && SWFDEC_IS_ACTOR (ret) && swfdec_actor_get_mouse_events (SWFDEC_ACTOR (ret)))
      return ret;
  }

  return movie;
}

static SwfdecTextFormat *
swfdec_text_field_movie_format_for_index (SwfdecTextFieldMovie *text,
    guint index_)
{
  GSList *iter;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), NULL);
  g_return_val_if_fail (index_ <= text->input->len, NULL);

  if (text->formats == NULL)
    return NULL;

  // get current format
  for (iter = text->formats; iter != NULL && iter->next != NULL &&
      ((SwfdecFormatIndex *)iter->next->data)->index_ < index_;
      iter = iter->next);

  return ((SwfdecFormatIndex *)iter->data)->format;
}

static void
swfdec_text_field_movie_letter_clicked (SwfdecTextFieldMovie *text,
    guint index_)
{
  SwfdecTextFormat *format;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (index_ <= text->input->len);

  format = swfdec_text_field_movie_format_for_index (text, index_);

  if (format != NULL && format->attr.url != NULL &&
      format->attr.url != SWFDEC_AS_STR_EMPTY) {
    swfdec_player_launch (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context),
	SWFDEC_LOADER_REQUEST_DEFAULT, format->attr.url, format->attr.target, NULL);
  }
}

static gboolean
swfdec_text_field_movie_xy_to_index (SwfdecTextFieldMovie *text, double x,
    double y, guint *index_, gboolean *before)
{
  SwfdecLayout *layouts;
  int i, layout_index, trailing, pixels;
  double layout_y, layout_x;
  gboolean direct;

  g_return_val_if_fail (index_ != NULL, FALSE);
  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), FALSE);

  *index_ = 0;
  if (before != NULL)
    *before = FALSE;

  layouts = swfdec_text_field_movie_get_layouts (text, NULL, NULL, NULL, NULL);

  if (layouts[0].layout == NULL)
    return FALSE;

  layout_y = y - EXTRA_MARGIN - text->extents.y0;
  layout_x = x - EXTRA_MARGIN - text->extents.x0;

  // take scrolling into account
  swfdec_text_field_movie_line_position (layouts,
      MIN (text->scroll, text->scroll_max), &pixels, NULL, NULL);
  layout_y += pixels;
  layout_x += MIN (text->hscroll, text->hscroll_max);

  i = 0;
  while (layout_y > layouts[i].height && layouts[i + 1].layout != NULL) {
    layout_y -= layouts[i].height;
    i++;
  }

  layout_x -= layouts[i].offset_x;

  direct = pango_layout_xy_to_index (layouts[i].layout, layout_x * PANGO_SCALE,
      layout_y * PANGO_SCALE, &layout_index, &trailing);

  *index_ = layouts[i].index_ + layout_index;
  if (before)
    *before = (trailing == 0);

  swfdec_text_field_movie_free_layouts (layouts);

  return direct;
}

static SwfdecMouseCursor
swfdec_text_field_movie_mouse_cursor (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  double x, y;
  guint index_;
  SwfdecTextFormat *format;

  swfdec_movie_get_mouse (SWFDEC_MOVIE (actor), &x, &y);

  if (swfdec_text_field_movie_xy_to_index (text, x, y, &index_, NULL)) {
    format = swfdec_text_field_movie_format_for_index (text, index_);
  } else {
    format = NULL;
  }

  if (format != NULL && format->attr.url != NULL &&
      format->attr.url != SWFDEC_AS_STR_EMPTY) {
    return SWFDEC_MOUSE_CURSOR_CLICK;
  } else if (text->editable || text->selectable) {
    return SWFDEC_MOUSE_CURSOR_TEXT;
  } else{
    return SWFDEC_MOUSE_CURSOR_NORMAL;
  }
}

static gboolean
swfdec_text_field_movie_mouse_events (SwfdecActor *actor)
{
  return TRUE;
}

static void
swfdec_text_field_movie_mouse_press (SwfdecActor *actor, guint button)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  double x, y;
  guint index_;
  gboolean direct, before;

  if (button != 0) {
    SWFDEC_FIXME ("implement popup menus, scrollwheel and middle mouse paste");
    return;
  }

  if (!text->selectable)
    return;

  swfdec_movie_get_mouse (SWFDEC_MOVIE (actor), &x, &y);

  direct = swfdec_text_field_movie_xy_to_index (text, x, y, &index_, &before);

  text->mouse_pressed = TRUE;
  if (!before && index_ < text->input->len)
    index_++;
  swfdec_text_field_movie_set_cursor (text, index_, index_);

  if (direct) {
    text->character_pressed = index_;
  } else {
    text->character_pressed = 0;
  }

  swfdec_player_grab_focus (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context), actor);
}

static void
swfdec_text_field_movie_mouse_move (SwfdecActor *actor, double x, double y)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  guint index_;
  gboolean direct, before;

  if (!text->selectable)
    return;

  if (!text->mouse_pressed)
    return;

  direct = swfdec_text_field_movie_xy_to_index (text, x, y, &index_, &before);

  if (!before && index_ < text->input->len)
    index_++;

  swfdec_text_field_movie_set_cursor (text, swfdec_text_field_movie_get_cursor (text), index_);
}

static void
swfdec_text_field_movie_mouse_release (SwfdecActor *actor, guint button)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  double x, y;
  guint index_;
  gboolean direct, before;

  if (button != 0) {
    SWFDEC_FIXME ("implement popup menus, scrollwheel and middle mouse paste");
    return;
  }

  swfdec_movie_get_mouse (SWFDEC_MOVIE (text), &x, &y);

  text->mouse_pressed = FALSE;

  direct = swfdec_text_field_movie_xy_to_index (text, x, y, &index_, &before);

  if (text->character_pressed != 0) {
    if (direct && text->character_pressed == index_ + 1 - (before ? 1 : 0)) {
      swfdec_text_field_movie_letter_clicked (text,
	  text->character_pressed - 1);
    }

    text->character_pressed = 0;
  }
}

static void
swfdec_text_field_movie_focus_in (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  
  if (text->editable)
    swfdec_movie_invalidate_last (SWFDEC_MOVIE (actor));
}

static void
swfdec_text_field_movie_focus_out (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  
  if (text->editable)
    swfdec_movie_invalidate_last (SWFDEC_MOVIE (actor));
}

static void
swfdec_text_field_movie_changed (SwfdecTextFieldMovie *text)
{
  text->changed++;
}

static void
swfdec_text_field_movie_key_press (SwfdecActor *actor, guint keycode, guint character)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  char insert[7];
  guint len;
  gsize start, end;
#define BACKWARD(text, _index) ((_index) == 0 ? 0 : (gsize) (g_utf8_prev_char ((text)->input->str + (_index)) - (text)->input->str))
#define FORWARD(text, _index) ((_index) == (text)->input->len ? (_index) : (gsize) (g_utf8_next_char ((text)->input->str + (_index)) - (text)->input->str))

  if (!text->editable)
    return;

  swfdec_text_field_movie_get_selection (text, &start, &end);

  switch (keycode) {
    case SWFDEC_KEY_LEFT:
      if (swfdec_text_field_movie_has_cursor (text)) {
	start = BACKWARD (text, start);
	swfdec_text_field_movie_set_cursor (text, start, start);
      } else {
	swfdec_text_field_movie_set_cursor (text, start, start);
      }
      return;
    case SWFDEC_KEY_RIGHT:
      if (swfdec_text_field_movie_has_cursor (text)) {
	start = FORWARD (text, start);
	swfdec_text_field_movie_set_cursor (text, start, start);
      } else {
	swfdec_text_field_movie_set_cursor (text, end, end);
      }
      return;
    case SWFDEC_KEY_BACKSPACE:
      if (swfdec_text_field_movie_has_cursor (text)) {
	start = BACKWARD (text, start);
      }
      if (start != end) {
	swfdec_sandbox_use (SWFDEC_MOVIE (text)->resource->sandbox);
	swfdec_text_field_movie_replace_text (text, start, end, "");
	swfdec_sandbox_unuse (SWFDEC_MOVIE (text)->resource->sandbox);
	swfdec_text_field_movie_changed (text);
      }
      return;
    case SWFDEC_KEY_DELETE:
      if (swfdec_text_field_movie_has_cursor (text)) {
	end = FORWARD (text, end);
      }
      if (start != end) {
	swfdec_sandbox_use (SWFDEC_MOVIE (text)->resource->sandbox);
	swfdec_text_field_movie_replace_text (text, start, end, "");
	swfdec_sandbox_unuse (SWFDEC_MOVIE (text)->resource->sandbox);
	swfdec_text_field_movie_changed (text);
      }
      return;
    default:
      break;
  }

  if (character == 0)
    return;
  len = g_unichar_to_utf8 (character, insert);
  if (len) {
    insert[len] = 0;
    swfdec_sandbox_use (SWFDEC_MOVIE (text)->resource->sandbox);
    swfdec_text_field_movie_replace_text (text, start, end, insert);
    swfdec_sandbox_unuse (SWFDEC_MOVIE (text)->resource->sandbox);
    swfdec_text_field_movie_changed (text);
  }
}

static void
swfdec_text_field_movie_key_release (SwfdecActor *actor, guint keycode, guint character)
{
  //SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
}

static void
swfdec_text_field_movie_class_init (SwfdecTextFieldMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);
  SwfdecActorClass *actor_class = SWFDEC_ACTOR_CLASS (g_class);

  object_class->dispose = swfdec_text_field_movie_dispose;

  asobject_class->mark = swfdec_text_field_movie_mark;

  movie_class->init_movie = swfdec_text_field_movie_init_movie;
  movie_class->finish_movie = swfdec_text_field_movie_finish_movie;
  movie_class->update_extents = swfdec_text_field_movie_update_extents;
  movie_class->render = swfdec_text_field_movie_render;
  movie_class->invalidate = swfdec_text_field_movie_invalidate;
  movie_class->contains = swfdec_text_field_movie_contains;

  actor_class->mouse_cursor = swfdec_text_field_movie_mouse_cursor;
  actor_class->mouse_events = swfdec_text_field_movie_mouse_events;
  actor_class->mouse_press = swfdec_text_field_movie_mouse_press;
  actor_class->mouse_release = swfdec_text_field_movie_mouse_release;
  actor_class->mouse_move = swfdec_text_field_movie_mouse_move;
  actor_class->focus_in = swfdec_text_field_movie_focus_in;
  actor_class->focus_out = swfdec_text_field_movie_focus_out;
  actor_class->key_press = swfdec_text_field_movie_key_press;
  actor_class->key_release = swfdec_text_field_movie_key_release;

  actor_class->iterate_start = swfdec_text_field_movie_iterate;
}

static void
swfdec_text_field_movie_init (SwfdecTextFieldMovie *text)
{
  text->input = g_string_new ("");
  text->scroll = 1;
  text->mouse_wheel_enabled = TRUE;
}

void
swfdec_text_field_movie_set_text_format (SwfdecTextFieldMovie *text,
    SwfdecTextFormat *format, guint start_index, guint end_index)
{
  SwfdecFormatIndex *findex, *findex_new, *findex_prev;
  guint findex_end_index;
  GSList *iter, *next;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (SWFDEC_IS_TEXT_FORMAT (format));
  g_return_if_fail (start_index < end_index);
  g_return_if_fail (end_index <= text->input->len);

  g_assert (text->formats != NULL);
  g_assert (text->formats->data != NULL);
  g_assert (((SwfdecFormatIndex *)text->formats->data)->index_ == 0);

  findex = NULL;
  for (iter = text->formats; iter != NULL &&
      ((SwfdecFormatIndex *)iter->data)->index_ < end_index;
      iter = next)
  {
    findex_prev = findex;
    next = iter->next;
    findex = iter->data;
    if (iter->next != NULL) {
      findex_end_index =
	((SwfdecFormatIndex *)iter->next->data)->index_;
    } else {
      findex_end_index = text->input->len;
    }

    if (findex_end_index <= start_index)
      continue;

    if (swfdec_text_format_equal_or_undefined (findex->format, format))
      continue;

    if (findex_end_index > end_index) {
      findex_new = g_new (SwfdecFormatIndex, 1);
      findex_new->index_ = end_index;
      findex_new->format = swfdec_text_format_copy (findex->format);
      if (findex_new->format == NULL) {
	g_free (findex_new);
	break;
      }

      iter = g_slist_insert (iter, findex_new, 1);
    }

    if (findex->index_ < start_index) {
      findex_new = g_new (SwfdecFormatIndex, 1);
      findex_new->index_ = start_index;
      findex_new->format = swfdec_text_format_copy (findex->format);
      if (findex_new->format == NULL) {
	g_free (findex_new);
	break;
      }
      swfdec_text_format_add (findex_new->format, format);

      iter = g_slist_insert (iter, findex_new, 1);
      findex = findex_new;
    } else {
      swfdec_text_format_add (findex->format, format);

      // if current format now equals previous one, remove current
      if (findex_prev != NULL &&
	  swfdec_text_format_equal (findex->format, findex_prev->format)) {
	text->formats = g_slist_remove (text->formats, findex);
	findex = findex_prev;
      }
    }

    // if current format now equals the next one, remove current
    if (findex_end_index <= end_index && next != NULL &&
	swfdec_text_format_equal (findex->format,
	  ((SwfdecFormatIndex *)next->data)->format))
    {
      ((SwfdecFormatIndex *)next->data)->index_ = findex->index_;
      text->formats = g_slist_remove (text->formats, findex);
      findex = findex_prev;
    }
  }
}

SwfdecTextFormat *
swfdec_text_field_movie_get_text_format (SwfdecTextFieldMovie *text,
    guint start_index, guint end_index)
{
  SwfdecTextFormat *format;
  GSList *iter;

  g_assert (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_assert (start_index < end_index);
  g_assert (end_index <= text->input->len);

  g_assert (text->formats != NULL);
  g_assert (text->formats->data != NULL);
  g_assert (((SwfdecFormatIndex *)text->formats->data)->index_ == 0);

  format = NULL;
  for (iter = text->formats; iter != NULL &&
      ((SwfdecFormatIndex *)iter->data)->index_ < end_index;
      iter = iter->next)
  {
    if (iter->next != NULL &&
	((SwfdecFormatIndex *)iter->next->data)->index_ <= start_index)
      continue;

    if (format == NULL) {
      swfdec_text_format_init_properties (SWFDEC_AS_OBJECT (text)->context);
      format =
	swfdec_text_format_copy (((SwfdecFormatIndex *)iter->data)->format);
    } else {
      swfdec_text_format_remove_different (format,
	  ((SwfdecFormatIndex *)iter->data)->format);
    }
  }

  return format;
}

static void
swfdec_text_field_movie_parse_listen_variable (SwfdecTextFieldMovie *text,
    const char *variable, SwfdecAsObject **object, const char **name)
{
  SwfdecAsContext *cx;
  SwfdecAsObject *parent;
  const char *p1, *p2;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (object != NULL);
  g_return_if_fail (name != NULL);

  *object = NULL;
  *name = NULL;

  if (SWFDEC_MOVIE (text)->parent == NULL)
    return;

  g_assert (SWFDEC_IS_AS_OBJECT (SWFDEC_MOVIE (text)->parent));
  cx = SWFDEC_AS_OBJECT (text)->context;
  parent = SWFDEC_AS_OBJECT (SWFDEC_MOVIE (text)->parent);

  p1 = strrchr (variable, '.');
  p2 = strrchr (variable, ':');
  if (p1 == NULL && p2 == NULL) {
    *object = parent;
    *name = variable;
  } else {
    if (p1 == NULL || (p2 != NULL && p2 > p1))
      p1 = p2;
    if (strlen (p1) == 1)
      return;
    *object = swfdec_action_lookup_object (cx, parent, variable, p1);
    if (*object == NULL)
      return;
    *name = swfdec_as_context_get_string (cx, p1 + 1);
  }
}

void
swfdec_text_field_movie_set_listen_variable_text (SwfdecTextFieldMovie *text,
    const char *value)
{
  SwfdecAsObject *object;
  const char *name;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (text->variable != NULL);
  g_return_if_fail (value != NULL);

  swfdec_text_field_movie_parse_listen_variable (text, text->variable,
      &object, &name);
  if (object != NULL) {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_STRING (&val, value);
    swfdec_as_object_set_variable (object, name, &val);
  }
}

static void
swfdec_text_field_movie_variable_listener_callback (SwfdecAsObject *object,
    const char *name, const SwfdecAsValue *val)
{
  SwfdecTextFieldMovie *text;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (object));

  text = SWFDEC_TEXT_FIELD_MOVIE (object);
  swfdec_text_field_movie_set_text (text,
      swfdec_as_value_to_string (object->context, val), text->html);
}

void
swfdec_text_field_movie_set_listen_variable (SwfdecTextFieldMovie *text,
    const char *value)
{
  SwfdecAsObject *object;
  const char *name;

  // FIXME: case-insensitive when v < 7?
  if (text->variable == value)
    return;

  if (text->variable != NULL) {
    swfdec_text_field_movie_parse_listen_variable (text, text->variable,
	&object, &name);
    if (object != NULL && SWFDEC_IS_MOVIE (object)) {
      swfdec_movie_remove_variable_listener (SWFDEC_MOVIE (object),
	  SWFDEC_AS_OBJECT (text), name,
	  swfdec_text_field_movie_variable_listener_callback);
    }
  }

  text->variable = value;

  if (value != NULL) {
    SwfdecAsValue val;

    swfdec_text_field_movie_parse_listen_variable (text, value, &object,
	&name);
    if (object != NULL && swfdec_as_object_get_variable (object, name, &val)) {
      swfdec_text_field_movie_set_text (text,
	  swfdec_as_value_to_string (SWFDEC_AS_OBJECT (text)->context, &val),
	  text->html);
    }
    if (object != NULL && SWFDEC_IS_MOVIE (object)) {
      swfdec_movie_add_variable_listener (SWFDEC_MOVIE (object),
	  SWFDEC_AS_OBJECT (text), name,
	  swfdec_text_field_movie_variable_listener_callback);
    }
  }
}

const char *
swfdec_text_field_movie_get_text (SwfdecTextFieldMovie *text)
{
  char *str, *p;

  str = g_strdup (text->input->str);

  // if input was orginally html, remove all \r
  if (text->input_html) {
    p = str;
    while ((p = strchr (p, '\r')) != NULL) {
      memmove (p, p + 1, strlen (p));
    }
  }

  // change all \n to \r
  p = str;
  while ((p = strchr (p, '\n')) != NULL) {
    *p = '\r';
  }

  return swfdec_as_context_give_string (SWFDEC_AS_OBJECT (text)->context, str);
}

void
swfdec_text_field_movie_replace_text (SwfdecTextFieldMovie *text,
    guint start_index, guint end_index, const char *str)
{
  SwfdecFormatIndex *findex;
  GSList *iter;
  gboolean first;
  gsize len;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (end_index <= text->input->len);
  g_return_if_fail (start_index <= end_index);
  g_return_if_fail (str != NULL);

  /* if there was a style sheet set when setting the text, modifications are
   * not allowed */
  if (text->style_sheet_input)
    return;

  len = strlen (str);
  first = TRUE;
  iter = text->formats; 
  while (iter) {
    findex = iter->data;
    iter = iter->next;

    /* remove formats of deleted text */
    if (findex->index_ >= start_index &&
	(end_index == text->input->len ||
	 (iter != NULL &&
	  ((SwfdecFormatIndex *) iter->data)->index_ <= end_index)) &&
	text->formats->next != NULL) {
      text->formats = g_slist_remove (text->formats, findex);
      g_free (findex);
      continue;
    }
    /* adapt indexes: remove deleted part, add to-be inserted text */
    if (findex->index_ > end_index) {
      findex->index_ = findex->index_ + start_index - end_index + len;
    } else if (findex->index_ > start_index) {
      findex->index_ = start_index;
    }
  }

  if (end_index == text->input->len && text->input->len > 0) {
    if (SWFDEC_AS_OBJECT (text)->context->version < 8) {
      SWFDEC_FIXME ("replaceText to the end of the TextField might use wrong text format on version 7");
    }
    findex = g_new0 (SwfdecFormatIndex, 1);
    findex->index_ = start_index;
    findex->format = swfdec_text_format_copy (
	((SwfdecFormatIndex *)text->formats->data)->format);
    text->formats = g_slist_append (text->formats, findex);
  }

  text->input = g_string_erase (text->input, start_index,
      end_index - start_index);
  text->input = g_string_insert (text->input, start_index, str);

  if (text->cursor_start >= start_index) {
    if (text->cursor_start <= end_index)
      text->cursor_start = start_index + len;
    else
      text->cursor_start += end_index - start_index + len;
  }
  if (text->cursor_end >= start_index) {
    if (text->cursor_end <= end_index)
      text->cursor_end = start_index + len;
    else
      text->cursor_end += end_index - start_index + len;
  }

  swfdec_movie_invalidate_last (SWFDEC_MOVIE (text));
  swfdec_text_field_movie_auto_size (text);
  swfdec_text_field_movie_update_scroll (text, TRUE);
}

void
swfdec_text_field_movie_set_text (SwfdecTextFieldMovie *text, const char *str,
    gboolean html)
{
  SwfdecFormatIndex *block;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (str != NULL);

  if (text->format_new == NULL) {
    text->input = g_string_truncate (text->input, 0);
    return;
  }

  // remove old formatting info
  g_slist_foreach (text->formats, (GFunc) g_free, NULL);
  g_slist_free (text->formats);
  text->formats = NULL;

  // add the default style
  if (html && SWFDEC_AS_OBJECT (text)->context->version < 8)
    swfdec_text_format_set_defaults (text->format_new);
  block = g_new (SwfdecFormatIndex, 1);
  block->index_ = 0;
  g_assert (SWFDEC_IS_TEXT_FORMAT (text->format_new));
  block->format = swfdec_text_format_copy (text->format_new);
  if (block->format == NULL) {
    g_free (block);
    text->input = g_string_truncate (text->input, 0);
    return;
  }
  text->formats = g_slist_prepend (text->formats, block);

  text->input_html = html;

  if (SWFDEC_AS_OBJECT (text)->context->version >= 7 &&
      text->style_sheet != NULL)
  {
    text->style_sheet_input = str;
    swfdec_text_field_movie_html_parse (text, str);
  }
  else
  {
    text->style_sheet_input = NULL;
    if (html) {
      swfdec_text_field_movie_html_parse (text, str);
    } else {
      text->input = g_string_assign (text->input, str);
    }
  }

  swfdec_movie_invalidate_last (SWFDEC_MOVIE (text));
  swfdec_text_field_movie_auto_size (text);
  swfdec_text_field_movie_update_scroll (text, TRUE);
}
