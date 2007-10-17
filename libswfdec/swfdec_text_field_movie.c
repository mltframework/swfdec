/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
 *                    Pekka Lampila <pekka.lampila@iki.fi>
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

#include "swfdec_text_field_movie.h"
#include "swfdec_as_context.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_interpret.h"
#include "swfdec_text_format.h"
#include "swfdec_xml.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecTextFieldMovie, swfdec_text_field_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_text_field_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  swfdec_rect_union (extents, extents, 
      &SWFDEC_GRAPHIC (SWFDEC_TEXT_FIELD_MOVIE (movie)->text)->extents);
}

static void
swfdec_text_paragraph_add_attribute (SwfdecParagraph *paragraph,
    PangoAttribute *attr)
{
  paragraph->attrs = g_list_append (paragraph->attrs,
      pango_attribute_copy (attr));
  pango_attr_list_insert (paragraph->attrs_list, attr);
}

static void
swfdec_text_paragraph_add_block_attributes (SwfdecParagraph *paragraph,
    int index_, SwfdecTextFormat *format)
{
  gint32 length, i;
  SwfdecBlock *block;
  SwfdecAsValue val;

  block = g_new0 (SwfdecBlock, 1);

  block->index_ = index_;

  switch (format->align) {
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
  }
  block->leading = format->leading * 20 * PANGO_SCALE;
  block->block_indent = format->block_indent * 20;
  block->left_margin = format->left_margin * 20;
  block->right_margin = format->right_margin * 20;

  length = swfdec_as_array_get_length (format->tab_stops);
  block->tab_stops = pango_tab_array_new (length, TRUE);
  for (i = 0; i < length; i++) {
    swfdec_as_array_get_value (format->tab_stops, i, &val);
    g_assert (SWFDEC_AS_VALUE_IS_NUMBER (&val));
    pango_tab_array_set_tab (block->tab_stops, i, PANGO_TAB_LEFT,
	SWFDEC_AS_VALUE_GET_NUMBER (&val) * 20);
  }

  paragraph->blocks = g_list_append (paragraph->blocks, block);
}

static void
swfdec_text_field_movie_generate_paragraph (SwfdecTextFieldMovie *text,
    SwfdecParagraph *paragraph, guint start_index, guint end_index)
{
  SwfdecTextFormat *format, *format_prev;
  guint index_;
  GSList *iter;
  PangoAttribute *attr_bold, *attr_color, *attr_font, *attr_italic,
		 *attr_letter_spacing, *attr_size, *attr_underline;
  // TODO: kerning, display

  g_assert (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_assert (paragraph != NULL);
  g_assert (start_index <= end_index);
  g_assert (end_index <= strlen (text->text_display));

  paragraph->text = text->text_display + start_index;
  paragraph->text_length = end_index - start_index;

  paragraph->blocks = NULL;
  paragraph->attrs = NULL;
  paragraph->attrs_list = pango_attr_list_new ();

  if (paragraph->text_length == 0)
    return;

  g_assert (text->formats != NULL);
  for (iter = text->formats; iter->next != NULL &&
      ((SwfdecFormatIndex *)(iter->next->data))->index <= start_index;
      iter = iter->next);

  index_ = start_index;
  format = ((SwfdecFormatIndex *)(iter->data))->format;

  // Paragraph formats
  paragraph->bullet = format->bullet;
  paragraph->indent = format->indent * 20 * PANGO_SCALE;

  // Add new block
  swfdec_text_paragraph_add_block_attributes (paragraph, 0, format);

  // Open attributes
  attr_bold = pango_attr_weight_new (
      (format->bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL));
  attr_bold->start_index = 0;

  attr_color = pango_attr_foreground_new (SWFDEC_COLOR_R (format->color) * 255,
      SWFDEC_COLOR_G (format->color) * 255,
      SWFDEC_COLOR_B (format->color) * 255);
  attr_color->start_index = 0;

  // FIXME: embed fonts
  attr_font = pango_attr_family_new (format->font);
  attr_font->start_index = 0;

  attr_italic = pango_attr_style_new (
      (format->italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
  attr_italic->start_index = 0;

  attr_letter_spacing = pango_attr_letter_spacing_new (
      format->letter_spacing * 20 * PANGO_SCALE);
  attr_letter_spacing->start_index = 0;

  attr_size =
    pango_attr_size_new_absolute (MAX (format->size, 1) * 20 * PANGO_SCALE);
  attr_size->start_index = 0;

  attr_underline = pango_attr_underline_new (
      (format->underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));
  attr_underline->start_index = 0;

  for (iter = iter->next;
      iter != NULL && ((SwfdecFormatIndex *)(iter->data))->index < end_index;
      iter = iter->next)
  {
    format_prev = format;
    index_ = ((SwfdecFormatIndex *)(iter->data))->index;
    format = ((SwfdecFormatIndex *)(iter->data))->format;

    // Add new block if necessary
    if (format_prev->align != format->align ||
       format_prev->bullet != format->bullet ||
       format_prev->indent != format->indent ||
       format_prev->leading != format->leading ||
       format_prev->block_indent != format->block_indent ||
       format_prev->left_margin != format->left_margin)
    {
      swfdec_text_paragraph_add_block_attributes (paragraph,
	  index_ - start_index, format);
    }

    // Change attributes if necessary
    if (format_prev->bold != format->bold) {
      attr_bold->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_bold);

      attr_bold = pango_attr_weight_new (
	  (format->bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL));
      attr_bold->start_index = index_ - start_index;
    }

    if (format_prev->color != format->color) {
      attr_color->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_color);

      attr_color = pango_attr_foreground_new (
	  SWFDEC_COLOR_R (format->color) * 255,
	  SWFDEC_COLOR_G (format->color) * 255,
	  SWFDEC_COLOR_B (format->color) * 255);
      attr_color->start_index = index_ - start_index;
    }

    if (format_prev->font != format->font) {
      attr_font->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_font);

      // FIXME: embed fonts
      attr_font = pango_attr_family_new (format->font);
      attr_font->start_index = index_ - start_index;
    }

    if (format_prev->italic != format->italic) {
      attr_italic->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_italic);

      attr_italic = pango_attr_style_new (
	  (format->italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
      attr_italic->start_index = index_ - start_index;
    }

    if (format_prev->letter_spacing != format->letter_spacing) {
      attr_letter_spacing->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_letter_spacing);

      attr_letter_spacing = pango_attr_letter_spacing_new (
	  format->letter_spacing * 20 * PANGO_SCALE);
      attr_letter_spacing->start_index = index_ - start_index;
    }

    if (format_prev->size != format->size) {
      attr_size->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_size);

      attr_size =
	pango_attr_size_new_absolute (MAX (1, format->size) * 20 * PANGO_SCALE);
      attr_size->start_index = index_ - start_index;
    }

    if (format_prev->underline != format->underline) {
      attr_underline->end_index = index_ - start_index;
      swfdec_text_paragraph_add_attribute (paragraph, attr_underline);

      attr_underline = pango_attr_underline_new (
	  (format->underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));
      attr_underline->start_index = index_ - start_index;
    }
  }

  // Close attributes
  attr_bold->end_index = end_index - start_index;
  swfdec_text_paragraph_add_attribute (paragraph, attr_bold);
  attr_bold = NULL;

  attr_color->end_index = end_index - start_index;
  swfdec_text_paragraph_add_attribute (paragraph, attr_color);
  attr_color = NULL;

  attr_font->end_index = end_index - start_index;
  swfdec_text_paragraph_add_attribute (paragraph, attr_font);
  attr_font = NULL;

  attr_italic->end_index = end_index - start_index;
  swfdec_text_paragraph_add_attribute (paragraph, attr_italic);
  attr_italic = NULL;

  attr_letter_spacing->end_index = end_index - start_index;
  swfdec_text_paragraph_add_attribute (paragraph, attr_letter_spacing);
  attr_letter_spacing = NULL;

  attr_size->end_index = end_index - start_index;
  swfdec_text_paragraph_add_attribute (paragraph, attr_size);
  attr_size = NULL;

  attr_underline->end_index = end_index - start_index;
  swfdec_text_paragraph_add_attribute (paragraph, attr_underline);
  attr_underline = NULL;
}

static void
swfdec_text_field_movie_generate_paragraphs (SwfdecTextFieldMovie *text)
{
  const char *p, *end;
  int num, i;

  g_assert (SWFDEC_IS_TEXT_FIELD_MOVIE (text));

  num = 0;
  p = text->text_display;
  while (p != NULL && *p != '\0') {
    num++;
    p = strchr (p, '\r');
    if (p != NULL) p++;
  }

  text->paragraphs = g_new0 (SwfdecParagraph, num + 1);

  i = 0;
  p = text->text_display;
  while (*p != '\0') {
    g_assert (i < num);
    end = strchr (p, '\r');
    if (end == NULL)
      end = strchr (p, '\0');

    swfdec_text_field_movie_generate_paragraph (text, &text->paragraphs[i],
	p - text->text_display, end - text->text_display);

    p = end;
    if (*p == '\r') p++;

    i++;
  }
  g_assert (i == num);
}

static void
swfdec_text_field_movie_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);

  if (text->paragraphs == NULL)
    swfdec_text_field_movie_generate_paragraphs (text);

  swfdec_text_field_render (text->text, cr, text->paragraphs,
      text->border_color, text->background_color, trans, inval);
}

static void
swfdec_text_field_movie_free_paragraphs (SwfdecTextFieldMovie *text)
{
  GList *iter;
  int i;

  if (text->paragraphs) {
    for (i = 0; text->paragraphs[i].text != NULL; i++)
    {
      for (iter = text->paragraphs[i].blocks; iter != NULL; iter = iter->next) {
	pango_tab_array_free (((SwfdecBlock *)(iter->data))->tab_stops);
      }
      g_list_free (text->paragraphs[i].blocks);

      for (iter = text->paragraphs[i].attrs; iter != NULL; iter = iter->next) {
	pango_attribute_destroy ((PangoAttribute *)(iter->data));
      }
      g_list_free (text->paragraphs[i].attrs);

      if (text->paragraphs[i].attrs_list != NULL)
	pango_attr_list_unref (text->paragraphs[i].attrs_list);
    }
    g_free (text->paragraphs);
    text->paragraphs = NULL;
  }
}

static SwfdecLayout *
swfdec_text_field_movie_get_layouts (SwfdecTextFieldMovie *text, int *num)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), NULL);

  if (text->paragraphs == NULL)
    swfdec_text_field_movie_generate_paragraphs (text);

  return swfdec_text_field_generate_layouts (text->text, text->cr,
      text->paragraphs, NULL, NULL, num);
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

void
swfdec_text_field_movie_set_scroll (SwfdecTextFieldMovie *text, int value)
{
  SwfdecLayout *layouts;
  int i, num, y, visible, all, height;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));

  layouts = swfdec_text_field_movie_get_layouts (text, &num);

  height = SWFDEC_GRAPHIC (text->text)->extents.y1 -
    SWFDEC_GRAPHIC (text->text)->extents.y0;
  y = 0;
  all = 0;
  visible = 0;

  for (i = num - 1; i >= 0; i--)
  {
    SwfdecLayout *layout = &layouts[i];
    PangoLayoutIter *iter_line;
    PangoRectangle rect;

    y += layout->height;

    iter_line = pango_layout_get_iter (layout->layout);

    do {
      pango_layout_iter_get_line_extents (iter_line, NULL, &rect);
      pango_extents_to_pixels (NULL, &rect);

      if (y - rect.y <= height)
	visible++;

      all++;
    } while (pango_layout_iter_next_line (iter_line));
  }

  swfdec_text_field_movie_free_layouts (layouts);

  if (value < 1) {
    value = 1;
  } else if (value > all - visible + 1) {
    value = all - visible + 1;
  }

  if (text->text->scroll != value) {
    text->text->scroll = value;
    swfdec_movie_invalidate (SWFDEC_MOVIE (text));
  }
}

static gboolean
swfdec_text_field_movie_auto_size (SwfdecTextFieldMovie *text)
{
  SwfdecLayout *layouts;
  guint height;
  int i, width, diff;
  gboolean changed;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), FALSE);

  if (text->text->auto_size == SWFDEC_AUTO_SIZE_NONE)
    return FALSE;

  layouts = swfdec_text_field_movie_get_layouts (text, NULL);

  width = 0;
  height = 3;
  for (i = 0; layouts[i].layout != NULL; i++) {
    if (!text->text->word_wrap) {
      if (layouts[i].width > width)
	width = layouts[i].width;
    }

    height += layouts[i].height;
  }

  swfdec_text_field_movie_free_layouts (layouts);

  if (!text->text->word_wrap && SWFDEC_GRAPHIC (text->text)->extents.x1 -
      SWFDEC_GRAPHIC (text->text)->extents.x0 != width)
  {
    switch (text->text->auto_size) {
      case SWFDEC_AUTO_SIZE_LEFT:
	SWFDEC_GRAPHIC (text->text)->extents.x1 =
	  SWFDEC_GRAPHIC (text->text)->extents.x0 + width;
	break;
      case SWFDEC_AUTO_SIZE_RIGHT:
	SWFDEC_GRAPHIC (text->text)->extents.x0 =
	  SWFDEC_GRAPHIC (text->text)->extents.x1 - width;
	break;
      case SWFDEC_AUTO_SIZE_CENTER:
	diff = width - (SWFDEC_GRAPHIC (text->text)->extents.x1 -
	  SWFDEC_GRAPHIC (text->text)->extents.x0);
	SWFDEC_GRAPHIC (text->text)->extents.x0 += floor (width / 2.0);
	SWFDEC_GRAPHIC (text->text)->extents.x1 += ceil (width / 2.0);
	break;
      default:
	g_assert_not_reached ();
    }
    changed = TRUE;
  }

  if (SWFDEC_GRAPHIC (text->text)->extents.y1 -
      SWFDEC_GRAPHIC (text->text)->extents.y0 != height)
  {
    SWFDEC_GRAPHIC (text->text)->extents.y1 =
      SWFDEC_GRAPHIC (text->text)->extents.y0 + height;
    changed = TRUE;
  }

  return changed;
}

void
swfdec_text_field_movie_format_changed (SwfdecTextFieldMovie *text)
{
  swfdec_text_field_movie_free_paragraphs (text);

  swfdec_movie_invalidate (SWFDEC_MOVIE (text));

  if (swfdec_text_field_movie_auto_size (text)) {
    swfdec_movie_queue_update (SWFDEC_MOVIE (text),
	SWFDEC_MOVIE_INVALID_CONTENTS);
  }
}

static void
swfdec_text_field_movie_dispose (GObject *object)
{
  SwfdecTextFieldMovie *text;
  GSList *iter;

  text = SWFDEC_TEXT_FIELD_MOVIE (object);

  swfdec_text_field_movie_free_paragraphs (text);

  for (iter = text->formats; iter != NULL; iter = iter->next) {
    g_free (text->formats->data);
    text->formats->data = NULL;
  }
  g_slist_free (text->formats);

  cairo_destroy (text->cr);
  cairo_surface_destroy (text->surface);

  G_OBJECT_CLASS (swfdec_text_field_movie_parent_class)->dispose (object);
}

static void
swfdec_text_field_movie_mark (SwfdecAsObject *object)
{
  SwfdecTextFieldMovie *text;
  GSList *iter;

  text = SWFDEC_TEXT_FIELD_MOVIE (object);

  if (text->text_input != NULL)
    swfdec_as_string_mark (text->text_input);
  swfdec_as_string_mark (text->text_display);
  if (text->variable != NULL)
    swfdec_as_string_mark (text->variable);
  swfdec_as_object_mark (SWFDEC_AS_OBJECT (text->format_new));
  for (iter = text->formats; iter != NULL; iter = iter->next) {
    swfdec_as_object_mark (
	SWFDEC_AS_OBJECT (((SwfdecFormatIndex *)(iter->data))->format));
  }
  swfdec_as_object_mark (SWFDEC_AS_OBJECT (text->format_new));
  if (text->style_sheet != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (text->style_sheet));
  if (text->restrict_ != NULL)
    swfdec_as_string_mark (text->restrict_);

  SWFDEC_AS_OBJECT_CLASS (swfdec_text_field_movie_parent_class)->mark (object);
}

static void
swfdec_text_field_movie_init_movie (SwfdecMovie *movie)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecAsContext *cx;
  SwfdecAsValue val;

  cx = SWFDEC_AS_OBJECT (movie)->context;

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextField, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (movie),
	SWFDEC_AS_VALUE_GET_OBJECT (&val));
  }

  // format
  text->format_new =
    SWFDEC_TEXT_FORMAT (swfdec_text_format_new_no_properties (cx));
  swfdec_text_format_set_defaults (text->format_new);
  text->format_new->color = text->text->color;
  text->format_new->align = text->text->align;
  if (text->text->font != NULL)  {
    text->format_new->font =
      swfdec_as_context_get_string (cx, text->text->font);
  }
  text->format_new->size = text->text->size / 20;
  text->format_new->left_margin = text->text->left_margin / 20;
  text->format_new->right_margin = text->text->right_margin / 20;
  text->format_new->indent = text->text->indent / 20;
  text->format_new->leading = text->text->leading / 20;

  text->border_color = SWFDEC_COLOR_COMBINE (0, 0, 0, 0);
  text->background_color = SWFDEC_COLOR_COMBINE (255, 255, 255, 0);

  // text
  if (text->text->text_input != NULL) {
    swfdec_text_field_movie_set_text (text,
	swfdec_as_context_get_string (cx, text->text->text_input),
	text->text->html);
  } else {
    swfdec_text_field_movie_set_text (text, SWFDEC_AS_STR_EMPTY,
	text->text->html);
  }

  // variable
  if (text->text->variable != NULL) {
    swfdec_text_field_movie_set_listen_variable (text,
	swfdec_as_context_get_string (cx, text->text->variable));
  }
}

static void
swfdec_text_field_movie_finish_movie (SwfdecMovie *movie)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);

  swfdec_text_field_movie_set_listen_variable (text, NULL);
}

static void
swfdec_text_field_movie_class_init (SwfdecTextFieldMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_text_field_movie_dispose;

  asobject_class->mark = swfdec_text_field_movie_mark;

  movie_class->init_movie = swfdec_text_field_movie_init_movie;
  movie_class->finish_movie = swfdec_text_field_movie_finish_movie;
  movie_class->update_extents = swfdec_text_field_movie_update_extents;
  movie_class->render = swfdec_text_field_movie_render;
}

static void
swfdec_text_field_movie_init (SwfdecTextFieldMovie *text)
{
  text->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
  text->cr = cairo_create (text->surface);

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
  g_return_if_fail (end_index <= strlen (text->text_display));

  g_assert (text->formats != NULL);
  g_assert (text->formats->data != NULL);
  g_assert (((SwfdecFormatIndex *)text->formats->data)->index == 0);

  findex = NULL;
  for (iter = text->formats; iter != NULL &&
      ((SwfdecFormatIndex *)iter->data)->index < end_index;
      iter = next)
  {
    findex_prev = findex;
    next = iter->next;
    findex = iter->data;
    if (iter->next != NULL) {
      findex_end_index =
	((SwfdecFormatIndex *)iter->next->data)->index;
    } else {
      findex_end_index = strlen (text->text_display);
    }

    if (findex_end_index < start_index)
      continue;

    if (swfdec_text_format_equal_or_undefined (findex->format, format))
      continue;

    if (findex_end_index > end_index) {
      findex_new = g_new (SwfdecFormatIndex, 1);
      findex_new->index = end_index;
      findex_new->format = swfdec_text_format_copy (findex->format);

      iter = g_slist_insert (iter, findex_new, 1);
    }

    if (findex->index < start_index) {
      findex_new = g_new (SwfdecFormatIndex, 1);
      findex_new->index = start_index;
      findex_new->format = swfdec_text_format_copy (findex->format);
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
      text->formats = g_slist_remove (text->formats, findex);
      findex = findex_prev;
    }
  }

  swfdec_text_field_movie_format_changed (text);
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
      swfdec_as_value_to_string (object->context, val), text->text->html);
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
	  text->text->html);
    }
    if (object != NULL && SWFDEC_IS_MOVIE (object)) {
      swfdec_movie_add_variable_listener (SWFDEC_MOVIE (object),
	  SWFDEC_AS_OBJECT (text), name,
	  swfdec_text_field_movie_variable_listener_callback);
    }
  }
}

static const char *
align_to_string (SwfdecTextAlign align)
{
  switch (align) {
    case SWFDEC_TEXT_ALIGN_LEFT:
      return "LEFT";
    case SWFDEC_TEXT_ALIGN_RIGHT:
      return "RIGHT";
    case SWFDEC_TEXT_ALIGN_CENTER:
      return "CENTER";
    case SWFDEC_TEXT_ALIGN_JUSTIFY:
      return "JUSTIFY";
    default:
      g_assert_not_reached ();
  }
}

/*
 * Order of tags:
 * TEXTFORMAT / P or LI / FONT / A / B / I / U
 *
 * Order of attributes:
 * TEXTFORMAT:
 * LEFTMARGIN / RIGHTMARGIN / INDENT / LEADING / BLOCKINDENT / TABSTOPS
 * P: ALIGN
 * LI: none
 * FONT: FACE / SIZE / COLOR / LETTERSPACING / KERNING
 * A: HREF / TARGET
 * B: none
 * I: none
 * U: none
 */
static GString *
swfdec_text_field_movie_html_text_append_paragraph (SwfdecTextFieldMovie *text,
    GString *string, guint start_index, guint end_index)
{
  SwfdecTextFormat *format, *format_prev, *format_font;
  GSList *iter, *fonts, *iter_font;
  guint index_, index_prev;
  gboolean textformat, bullet, font;
  char *escaped;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), string);
  g_return_val_if_fail (string != NULL, string);
  g_return_val_if_fail (start_index < end_index, string);

  g_return_val_if_fail (text->formats != NULL, string);
  for (iter = text->formats; iter->next != NULL &&
      ((SwfdecFormatIndex *)(iter->next->data))->index <= start_index;
      iter = iter->next);

  index_ = start_index;
  format = ((SwfdecFormatIndex *)(iter->data))->format;

  if (format->left_margin != 0 || format->right_margin != 0 ||
      format->indent != 0 || format->leading != 0 ||
      format->block_indent != 0 ||
      swfdec_as_array_get_length (format->tab_stops) > 0)
  {
    string = g_string_append (string, "<TEXTFORMAT");
    if (format->left_margin) {
      g_string_append_printf (string, " LEFTMARGIN=\"%i\"",
	  format->left_margin);
    }
    if (format->right_margin) {
      g_string_append_printf (string, " RIGHTMARGIN=\"%i\"",
	  format->right_margin);
    }
    if (format->indent)
      g_string_append_printf (string, " INDENT=\"%i\"", format->indent);
    if (format->leading)
      g_string_append_printf (string, " LEADING=\"%i\"", format->leading);
    if (format->block_indent) {
      g_string_append_printf (string, " BLOCKINDENT=\"%i\"",
	  format->block_indent);
    }
    if (swfdec_as_array_get_length (format->tab_stops) > 0) {
      SwfdecAsValue val;
      SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT  (format->tab_stops));
      g_string_append_printf (string, " TABSTOPS=\"%s\"",
	  swfdec_as_value_to_string (SWFDEC_AS_OBJECT
	    (format->tab_stops)->context, &val));
    }
    string = g_string_append (string, ">");

    textformat = TRUE;
  }
  else
  {
    textformat = FALSE;
  }

  if (format->bullet) {
    string = g_string_append (string, "<LI>");
    bullet = TRUE;
  } else {
    g_string_append_printf (string, "<P ALIGN=\"%s\">",
	align_to_string (format->align));
    bullet = FALSE;
  }

  // note we don't escape format->font, even thought it can have evil chars
  g_string_append_printf (string, "<FONT FACE=\"%s\" SIZE=\"%i\" COLOR=\"#%06X\" LETTERSPACING=\"%i\" KERNING=\"%i\">",
      format->font, format->size, format->color, (int)format->letter_spacing,
      (format->kerning ? 1 : 0));
  fonts = g_slist_prepend (NULL, format);

  if (format->url != SWFDEC_AS_STR_EMPTY)
    g_string_append_printf (string, "<A HREF=\"%s\" TARGET=\"%s\">",
	format->url, format->target);
  if (format->bold)
    string = g_string_append (string, "<B>");
  if (format->italic)
    string = g_string_append (string, "<I>");
  if (format->underline)
    string = g_string_append (string, "<U>");

  // special case: use <= instead of < to add some extra markup
  for (iter = iter->next;
      iter != NULL && ((SwfdecFormatIndex *)(iter->data))->index <= end_index;
      iter = iter->next)
  {
    index_prev = index_;
    format_prev = format;
    index_ = ((SwfdecFormatIndex *)(iter->data))->index;
    format = ((SwfdecFormatIndex *)(iter->data))->format;

    escaped = swfdec_xml_escape_len (text->text_display + index_prev,
	index_ - index_prev);
    string = g_string_append (string, escaped);
    g_free (escaped);
    escaped = NULL;

    // Figure out what tags need to be rewritten
    if (format->font != format_prev->font ||
	format->size != format_prev->size ||
	format->color != format_prev->color ||
	(int)format->letter_spacing != (int)format_prev->letter_spacing ||
	format->kerning != format_prev->kerning) {
      font = TRUE;
    } else if (format->url == format_prev->url &&
	format->target == format_prev->target &&
	format->bold == format_prev->bold &&
	format->italic == format_prev->italic &&
	format->underline == format_prev->underline) {
      continue;
    }

    // Close tags
    for (iter_font = fonts; iter_font != NULL; iter_font = iter_font->next)
    {
      format_font = (SwfdecTextFormat *)iter_font->data;
      if (format->font == format_font->font &&
	format->size == format_font->size &&
	format->color == format_font->color &&
	(int)format->letter_spacing == (int)format_font->letter_spacing &&
	format->kerning == format_font->kerning) {
	break;
      }
    }
    if (iter_font != NULL) {
      while (fonts != iter_font) {
	string = g_string_append (string, "</FONT>");
	fonts = g_slist_remove (fonts, fonts->data);
      }
    }
    if (format_prev->underline)
      string = g_string_append (string, "</U>");
    if (format_prev->italic)
      string = g_string_append (string, "</I>");
    if (format_prev->bold)
      string = g_string_append (string, "</B>");
    if (format_prev->url != SWFDEC_AS_STR_EMPTY)
      string = g_string_append (string, "</A>");

    // Open tags
    format_font = (SwfdecTextFormat *)fonts->data;
    if (font && (format->font != format_font->font ||
	 format->size != format_font->size ||
	 format->color != format_font->color ||
	 (int)format->letter_spacing != (int)format_font->letter_spacing ||
	 format->kerning != format_font->kerning))
    {
      fonts = g_slist_prepend (fonts, format);

      string = g_string_append (string, "<FONT");
      // note we don't escape format->font, even thought it can have evil chars
      if (format->font != format_font->font)
	g_string_append_printf (string, " FACE=\"%s\"", format->font);
      if (format->size != format_font->size)
	g_string_append_printf (string, " SIZE=\"%i\"", format->size);
      if (format->color != format_font->color)
	g_string_append_printf (string, " COLOR=\"#%06X\"", format->color);
      if ((int)format->letter_spacing != (int)format_font->letter_spacing) {
	g_string_append_printf (string, " LETTERSPACING=\"%i\"",
	    (int)format->letter_spacing);
      }
      if (format->kerning != format_font->kerning) {
	g_string_append_printf (string, " KERNING=\"%i\"",
	    (format->kerning ? 1 : 0));
      }
      string = g_string_append (string, ">");
    }
    if (format->url != SWFDEC_AS_STR_EMPTY) {
      g_string_append_printf (string, "<A HREF=\"%s\" TARGET=\"%s\">",
	  format->url, format->target);
    }
    if (format->bold)
      string = g_string_append (string, "<B>");
    if (format->italic)
      string = g_string_append (string, "<I>");
    if (format->underline)
      string = g_string_append (string, "<U>");
  }

  escaped = swfdec_xml_escape_len (text->text_display + index_,
      end_index - index_);
  string = g_string_append (string, escaped);
  g_free (escaped);

  if (format->underline)
    string = g_string_append (string, "</U>");
  if (format->italic)
    string = g_string_append (string, "</I>");
  if (format->bold)
    string = g_string_append (string, "</B>");
  if (format->url != SWFDEC_AS_STR_EMPTY)
    string = g_string_append (string, "</A>");
  for (iter = fonts; iter != NULL; iter = iter->next)
    string = g_string_append (string, "</FONT>");
  g_slist_free (fonts);
  if (bullet) {
    string = g_string_append (string, "</LI>");
  } else {
    string = g_string_append (string, "</P>");
  }
  if (textformat)
    string = g_string_append (string, "</TEXTFORMAT>");

  return string;
}

const char *
swfdec_text_field_movie_get_html_text (SwfdecTextFieldMovie *text)
{
  const char *p, *end;
  GString *string;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text),
      SWFDEC_AS_STR_EMPTY);

  if (text->text_display == NULL)
    return SWFDEC_AS_STR_EMPTY;

  if (text->text->html == FALSE)
    return text->text_display;

  string = g_string_new ("");

  p = text->text_display;
  while (*p != '\0') {
    end = strchr (p, '\r');
    if (end == NULL)
      end = strchr (p, '\0');

    string = swfdec_text_field_movie_html_text_append_paragraph (text, string,
	p - text->text_display, end - text->text_display);

    if (*end == '\r') {
      p = end + 1;
    } else {
      p = end;
    }
  }

  return swfdec_as_context_give_string (SWFDEC_AS_OBJECT (text)->context,
      g_string_free (string, FALSE));
}

void
swfdec_text_field_movie_set_text (SwfdecTextFieldMovie *text, const char *str,
    gboolean html)
{
  SwfdecFormatIndex *block;
  GSList *iter;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (str != NULL);

  text->text_input = str;

  // remove old formatting info
  iter = text->formats;
  while (iter) {
    g_free (iter->data);
    iter = g_slist_next (iter);
  }
  g_slist_free (text->formats);
  text->formats = NULL;

  // add the default style
  if (html && SWFDEC_AS_OBJECT (text)->context->version < 8)
    swfdec_text_format_set_defaults (text->format_new);
  block = g_new (SwfdecFormatIndex, 1);
  block->index = 0;
  g_assert (SWFDEC_IS_TEXT_FORMAT (text->format_new));
  block->format = text->format_new;
  text->formats = g_slist_prepend (text->formats, block);

  if (html) {
    swfdec_text_field_movie_html_parse (text, str);
  } else {
    // change all \n to \r
    if (strchr (str, '\n') != NULL) {
      char *string, *p;

      string = g_strdup (str);
      p = string;
      while ((p = strchr (p, '\n')) != NULL) {
	*p = '\r';
      }
      text->text_display = swfdec_as_context_give_string (
	  SWFDEC_AS_OBJECT (text)->context, string);
    } else {
      text->text_display = str;
    }
  }

  swfdec_text_field_movie_format_changed (text);
}
