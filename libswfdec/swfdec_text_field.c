/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include <pango/pangocairo.h>
#include <string.h>

#include "swfdec_text_field.h"
#include "swfdec_debug.h"
#include "swfdec_text_field_movie.h"
#include "swfdec_font.h"
#include "swfdec_player_internal.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecTextField, swfdec_text_field, SWFDEC_TYPE_GRAPHIC)

static gboolean
swfdec_text_field_mouse_in (SwfdecGraphic *graphic, double x, double y)
{
  return swfdec_rect_contains (&graphic->extents, x, y);
}

static SwfdecMovie *
swfdec_text_field_create_movie (SwfdecGraphic *graphic, gsize *size)
{
  SwfdecTextField *text = SWFDEC_TEXT_FIELD (graphic);
  SwfdecTextFieldMovie *ret = g_object_new (SWFDEC_TYPE_TEXT_FIELD_MOVIE, NULL);

  ret->text = text;

  *size = sizeof (SwfdecTextFieldMovie);

  return SWFDEC_MOVIE (ret);
}

static void
swfdec_text_field_dispose (GObject *object)
{
  SwfdecTextField *text = SWFDEC_TEXT_FIELD (object);

  g_free (text->text_input);
  text->text_input = NULL;
  g_free (text->variable);
  text->variable = NULL;

  G_OBJECT_CLASS (swfdec_text_field_parent_class)->dispose (object);
}

static void
swfdec_text_field_class_init (SwfdecTextFieldClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);

  object_class->dispose = swfdec_text_field_dispose;
  graphic_class->create_movie = swfdec_text_field_create_movie;
  graphic_class->mouse_in = swfdec_text_field_mouse_in;
}

static void
swfdec_text_field_init (SwfdecTextField * text)
{
  text->max_length = G_MAXUINT;
}

GList *
swfdec_text_field_generate_layouts (SwfdecTextField *text, cairo_t *cr,
    const SwfdecParagraph *paragraphs, const SwfdecColorTransform *trans,
    const SwfdecRect *inval)
{
  GList *layouts;
  guint i;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD (text), NULL);
  g_return_val_if_fail (cr != NULL, NULL);
  g_return_val_if_fail (paragraphs != NULL, NULL);

  if (paragraphs[0].text == NULL)
    return NULL;

  layouts = NULL;
  for (i = 0; paragraphs[i].text != NULL; i++)
  {
    GList *iter;
    guint skip;

    skip = 0;
    for (iter = paragraphs[i].blocks; iter != NULL; iter = iter->next)
    {
      SwfdecLayout *layout;
      PangoLayout *playout;
      int width;
      guint length;
      SwfdecBlock *block;

      block = (SwfdecBlock *)iter->data;
      if (iter->next != NULL) {
	length =
	  ((SwfdecBlock *)(iter->next->data))->index_ - block->index_;
      } else {
	length = paragraphs[i].text_length - block->index_;
      }

      if (skip > length) {
	skip -= length;
	continue;
      }

      // create layout
      layout = g_new0 (SwfdecLayout, 1);
      playout = layout->layout = pango_cairo_create_layout (cr);
      layouts = g_list_append (layouts, layout);

      // set rendering position
      layout->render_offset_x = block->left_margin + block->block_indent;
      width = SWFDEC_GRAPHIC (text)->extents.x1 -
	SWFDEC_GRAPHIC (text)->extents.x0 - block->left_margin -
	block->right_margin - block->block_indent;

      if (block->index_ == 0 && paragraphs[i].indent < 0) {
	layout->render_offset_x += paragraphs[i].indent / PANGO_SCALE;
	width += -paragraphs[i].indent / PANGO_SCALE;
      }

      if (text->word_wrap) {
	pango_layout_set_wrap (playout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_width (playout, width * PANGO_SCALE);
      } else {
	pango_layout_set_width (playout, -1);
      }

      // set paragraph styles
      if (block->index_ == 0) {
	pango_layout_set_indent (playout, paragraphs[i].indent);
	// TODO: bullet
      } else {
	pango_layout_set_indent (playout, 0);
      }

      // set block styles
      pango_layout_set_alignment (playout, block->align);
      pango_layout_set_justify (playout, block->justify);
      pango_layout_set_spacing (playout, block->leading);
      pango_layout_set_tabs (playout, block->tab_stops);

      // set text attributes
      if (block->index_ > 0 ||
	  (trans != NULL && !swfdec_color_transform_is_identity (trans)))
      {
	PangoAttrList *attr_list;
	GList *iter_attrs;

	attr_list = pango_attr_list_new ();

	for (iter_attrs = paragraphs[i].attrs; iter_attrs != NULL;
	    iter_attrs = iter_attrs->next)
	{
	  PangoAttribute *attr;

	  attr = (PangoAttribute *)iter_attrs->data;

	  if (attr->end_index <= block->index_ + skip)
	    continue;

	  attr = pango_attribute_copy (attr);
	  if (attr->klass->type == PANGO_ATTR_FOREGROUND && trans != NULL &&
	      !swfdec_color_transform_is_identity (trans))
	  {
	    SwfdecColor color;
	    PangoColor pcolor;

	    pcolor = ((PangoAttrColor *)attr)->color;
	    color = SWFDEC_COLOR_COMBINE (pcolor.red >> 8, pcolor.green >> 8,
		pcolor.blue >> 8, 255);
	    color = swfdec_color_apply_transform (color, trans);
	    pcolor.red = SWFDEC_COLOR_R (color) << 8;
	    pcolor.green = SWFDEC_COLOR_G (color) << 8;
	    pcolor.blue = SWFDEC_COLOR_B (color) << 8;
	    ((PangoAttrColor *)attr)->color = pcolor;
	  }
	  attr->start_index = (attr->start_index > block->index_ + skip ?
	      attr->start_index - (block->index_ + skip) : 0);
	  attr->end_index = attr->end_index - (block->index_ + skip);
	  pango_attr_list_insert (attr_list, attr);
	}
	pango_layout_set_attributes (playout, attr_list);
	pango_attr_list_unref (attr_list);
      } else {
	pango_layout_set_attributes (playout, paragraphs[i].attrs_list);
      }

      pango_layout_set_text (playout,
	  paragraphs[i].text + block->index_ + skip,
	  paragraphs[i].text_length - block->index_ - skip);

      if (iter->next != NULL && text->word_wrap)
      {
	PangoLayoutLine *line;
	int line_num;
	guint skip_new;

	pango_layout_index_to_line_x (playout, length - skip, FALSE, &line_num,
	    NULL);
	line = pango_layout_get_line_readonly (playout, line_num);
	skip_new = line->start_index + line->length - (length - skip);
	pango_layout_set_text (playout,
	    paragraphs[i].text + block->index_ + skip,
	    length - skip + skip_new);
	skip = skip_new;
      }
      else
      {
	skip = 0;
      }

      pango_layout_get_pixel_size (playout, &layout->width, &layout->height);
      layout->width += layout->render_offset_x + block->right_margin;
      layout->height += block->leading / PANGO_SCALE;

      if (!text->word_wrap)
	break;
    }
  }

  return layouts;
}

void
swfdec_text_field_render (SwfdecTextField *text, cairo_t *cr,
    const SwfdecParagraph *paragraphs, SwfdecColor border_color,
    SwfdecColor background_color, const SwfdecColorTransform *trans,
    const SwfdecRect *inval)
{
  GList *layouts, *iter;
  SwfdecRect limit;
  SwfdecColor color;
  int y;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD (text));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (paragraphs != NULL);
  g_return_if_fail (trans != NULL);
  g_return_if_fail (inval != NULL);

  swfdec_rect_intersect (&limit, &SWFDEC_GRAPHIC (text)->extents, inval);

  if (text->background) {
    cairo_rectangle (cr, limit.x0, limit.y0, limit.x1, limit.y1);
    color = swfdec_color_apply_transform (background_color, trans);
    swfdec_color_set_source (cr, color);
    cairo_fill (cr);
  }

  if (text->border) {
    cairo_rectangle (cr, SWFDEC_GRAPHIC (text)->extents.x0,
	SWFDEC_GRAPHIC (text)->extents.y0, SWFDEC_GRAPHIC (text)->extents.x1,
	SWFDEC_GRAPHIC (text)->extents.y1);
    color = swfdec_color_apply_transform (border_color, trans);
    swfdec_color_set_source (cr, color);
    cairo_set_line_width (cr, 20.0); // FIXME: Is this correct?
    cairo_stroke (cr);
  }

  layouts = swfdec_text_field_generate_layouts (text, cr, paragraphs, trans,
      inval);

  y = SWFDEC_GRAPHIC (text)->extents.y0 + 1;
  cairo_move_to (cr, SWFDEC_GRAPHIC (text)->extents.x0, y);

  for (iter = layouts; iter != NULL; iter = iter->next)
  {
    SwfdecLayout *layout = (SwfdecLayout *)iter->data;

    if (y + layout->height < limit.y0) {
      // no need to render
      cairo_rel_move_to (cr, 0, layout->height);
      y += layout->height;
    } else if (y < limit.y0 || y + layout->height > limit.y1) {
      // possibly skip some lines
      PangoLayoutIter *iter_line;
      PangoLayoutLine *line;
      PangoRectangle rect;

      cairo_rel_move_to (cr, layout->render_offset_x, 0);

      iter_line = pango_layout_get_iter (layout->layout);

      pango_layout_iter_get_line_extents (iter_line, NULL, &rect);
      pango_extents_to_pixels (NULL, &rect);
      while (y + rect.y + rect.height < limit.y0)
      {
	if (!pango_layout_iter_next_line (iter_line))
	  g_assert_not_reached ();
	pango_layout_iter_get_line_extents (iter_line, NULL, &rect);
	pango_extents_to_pixels (NULL, &rect);
      }

      while (y + rect.y <= limit.y1 &&
	  y + rect.y + rect.height <= SWFDEC_GRAPHIC (text)->extents.y1)
      {
	cairo_rel_move_to (cr, 0,
	    pango_layout_iter_get_baseline (iter_line) / PANGO_SCALE);
	line = pango_layout_iter_get_line_readonly (iter_line);
	pango_cairo_show_layout_line (cr, line);
	cairo_rel_move_to (cr, 0,
	    -pango_layout_iter_get_baseline (iter_line) / PANGO_SCALE);

	if (!pango_layout_iter_next_line (iter_line))
	  break;

	pango_layout_iter_get_line_extents (iter_line, NULL, &rect);
	pango_extents_to_pixels (NULL, &rect);
      }

      pango_layout_iter_free (iter_line);

      cairo_rel_move_to (cr, -layout->render_offset_x, layout->height);
      y += layout->height;

      if (y > limit.y1)
	break;
    } else {
      // render the whole layout
      cairo_rel_move_to (cr, layout->render_offset_x, 0);
      pango_cairo_show_layout (cr, layout->layout);
      cairo_rel_move_to (cr, -layout->render_offset_x, layout->height);
      y += layout->height;
    }
  }

  for (iter = layouts; iter != NULL; iter = iter->next)
  {
    SwfdecLayout *layout = (SwfdecLayout *)iter->data;

    g_object_unref (layout->layout);
    g_free (layout);
    iter->data = NULL;
  }

  g_list_free (layouts);
}


int
tag_func_define_edit_text (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecTextField *text;
  guint id;
  int reserved;
  gboolean has_font, has_color, has_max_length, has_layout, has_text;
  SwfdecBits *b = &s->b;

  id = swfdec_bits_get_u16 (b);
  SWFDEC_LOG ("  id = %u", id);
  text = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_TEXT_FIELD);
  if (text == NULL)
    return SWFDEC_STATUS_OK;

  swfdec_bits_get_rect (b, &SWFDEC_GRAPHIC (text)->extents);
  SWFDEC_LOG ("  extents: %g %g  %g %g", 
      SWFDEC_GRAPHIC (text)->extents.x0, SWFDEC_GRAPHIC (text)->extents.y0,
      SWFDEC_GRAPHIC (text)->extents.x1, SWFDEC_GRAPHIC (text)->extents.y1);
  swfdec_bits_syncbits (b);
  has_text = swfdec_bits_getbit (b);
  text->word_wrap = swfdec_bits_getbit (b);
  text->multiline = swfdec_bits_getbit (b);
  text->password = swfdec_bits_getbit (b);
  text->input = !swfdec_bits_getbit (b);
  has_color = swfdec_bits_getbit (b);
  has_max_length = swfdec_bits_getbit (b);
  has_font = swfdec_bits_getbit (b);
  reserved = swfdec_bits_getbit (b);
  text->auto_size =
    (swfdec_bits_getbit (b) ? SWFDEC_AUTO_SIZE_LEFT : SWFDEC_AUTO_SIZE_NONE);
  has_layout = swfdec_bits_getbit (b);
  text->selectable = !swfdec_bits_getbit (b);
  text->background = text->border = swfdec_bits_getbit (b);
  reserved = swfdec_bits_getbit (b);
  text->html = swfdec_bits_getbit (b);
  text->embed_fonts = swfdec_bits_getbit (b);
  if (text->embed_fonts)
    SWFDEC_FIXME ("Using embed fonts in TextField is not supported");
  if (has_font) {
    SwfdecCharacter *font;

    id = swfdec_bits_get_u16 (b);
    font = swfdec_swf_decoder_get_character (s, id);
    if (SWFDEC_IS_FONT (font)) {
      SWFDEC_LOG ("  font = %u", id);
      text->font = SWFDEC_FONT (font);
    } else {
      SWFDEC_ERROR ("id %u does not specify a font", id);
    }
    text->size = swfdec_bits_get_u16 (b);
    SWFDEC_LOG ("  size = %u", text->size);
  }
  if (has_color) {
    text->color = swfdec_bits_get_rgba (b);
    SWFDEC_LOG ("  color = %u", text->color);
  } else {
    SWFDEC_WARNING ("FIXME: figure out default color");
    text->color = SWFDEC_COLOR_COMBINE (255, 255, 255, 255);
  }
  if (has_max_length) {
    text->max_length = swfdec_bits_get_u16 (b);
  }
  if (has_layout) {
    guint align = swfdec_bits_get_u8 (b);
    switch (align) {
      case 0:
	text->align = SWFDEC_TEXT_ALIGN_LEFT;
	break;
      case 1:
	text->align = SWFDEC_TEXT_ALIGN_RIGHT;
	break;
      case 2:
	text->align = SWFDEC_TEXT_ALIGN_CENTER;
	break;
      case 3:
	text->align = SWFDEC_TEXT_ALIGN_JUSTIFY;
	break;
      default:
	SWFDEC_ERROR ("undefined align value %u", align);
	break;
    }
    text->left_margin = swfdec_bits_get_u16 (b);
    text->right_margin = swfdec_bits_get_u16 (b);
    text->indent = swfdec_bits_get_u16 (b);
    text->leading = swfdec_bits_get_s16 (b);
  }
  text->variable = swfdec_bits_get_string (b);
  if (text->variable && *text->variable == 0) {
    g_free (text->variable);
    text->variable = NULL;
  }
  if (has_text)
    text->text_input = swfdec_bits_get_string (b);

  return SWFDEC_STATUS_OK;
}
