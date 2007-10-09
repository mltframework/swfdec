/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_edittext.h"
#include "swfdec_debug.h"
#include "swfdec_edittext_movie.h"
#include "swfdec_font.h"
#include "swfdec_player_internal.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecEditText, swfdec_edit_text, SWFDEC_TYPE_GRAPHIC)

static gboolean
swfdec_edit_text_mouse_in (SwfdecGraphic *graphic, double x, double y)
{
  return swfdec_rect_contains (&graphic->extents, x, y);
}

static SwfdecMovie *
swfdec_edit_text_create_movie (SwfdecGraphic *graphic, gsize *size)
{
  SwfdecEditText *text = SWFDEC_EDIT_TEXT (graphic);
  SwfdecEditTextMovie *ret = g_object_new (SWFDEC_TYPE_EDIT_TEXT_MOVIE, NULL);

  ret->text = text;

  *size = sizeof (SwfdecEditTextMovie);

  return SWFDEC_MOVIE (ret);
}

static void
swfdec_edit_text_dispose (GObject *object)
{
  SwfdecEditText *text = SWFDEC_EDIT_TEXT (object);

  g_free (text->text_input);
  text->text_input = NULL;
  g_free (text->variable);
  text->variable = NULL;

  G_OBJECT_CLASS (swfdec_edit_text_parent_class)->dispose (object);
}

static void
swfdec_edit_text_class_init (SwfdecEditTextClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);

  object_class->dispose = swfdec_edit_text_dispose;
  graphic_class->create_movie = swfdec_edit_text_create_movie;
  graphic_class->mouse_in = swfdec_edit_text_mouse_in;
}

static void
swfdec_edit_text_init (SwfdecEditText * text)
{
  text->max_length = G_MAXUINT;
}

void
swfdec_edit_text_render (SwfdecEditText *text, cairo_t *cr,
    const SwfdecTextRenderBlock *blocks, const SwfdecColorTransform *trans,
    const SwfdecRect *inval, gboolean fill)
{
  guint i, j;
  PangoLayout *layout;
  PangoAttrList *attrs;
  guint width;
  int height, extra_chars, line_num;
  PangoLayoutLine *line;
  //SwfdecColor color;

  g_return_if_fail (SWFDEC_IS_EDIT_TEXT (text));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (blocks != NULL);
  g_return_if_fail (trans != NULL);
  g_return_if_fail (inval != NULL);

  cairo_move_to (cr, SWFDEC_GRAPHIC (text)->extents.x0,
      SWFDEC_GRAPHIC (text)->extents.y0);

  /*color = swfdec_color_apply_transform (text->color, trans);
  swfdec_color_set_source (cr, color);*/

  layout = pango_cairo_create_layout (cr);

  extra_chars = 0;
  for (i = 0; blocks[i].text != NULL; i++) {
    cairo_rel_move_to (cr, blocks[i].left_margin + blocks[i].block_indent, 0);
    width = SWFDEC_GRAPHIC (text)->extents.x1 -
      SWFDEC_GRAPHIC (text)->extents.x0 - blocks[i].left_margin -
      blocks[i].right_margin - blocks[i].block_indent;

    pango_cairo_update_layout (cr, layout);
    pango_layout_set_width (layout, width * PANGO_SCALE);

    // TODO: bullet

    pango_layout_set_alignment (layout, blocks[i].align);
    pango_layout_set_justify (layout, blocks[i].justify);
    pango_layout_set_indent (layout, blocks[i].indent);
    pango_layout_set_spacing (layout, blocks[i].leading);
    pango_layout_set_tabs (layout, blocks[i].tab_stops);

    if (!blocks[i].end_paragraph) {
      int prev_extra_chars = extra_chars;
      extra_chars = 0;
      attrs = pango_attr_list_copy (blocks[i].attrs);
      for (j = i + 1; blocks[j].text != NULL && !blocks[j-1].end_paragraph; j++)
      {
	pango_attr_list_splice (attrs, blocks[j].attrs,
	    blocks[i].text_length + extra_chars, blocks[j].text_length);
	extra_chars += blocks[j].text_length;
      }
      pango_layout_set_text (layout, blocks[i].text + prev_extra_chars,
	  blocks[i].text_length + extra_chars - prev_extra_chars);
      pango_layout_set_attributes (layout, attrs);
      pango_attr_list_unref (attrs);
      attrs = NULL;

      pango_layout_index_to_line_x (layout,
	  blocks[i].text_length - prev_extra_chars, FALSE, &line_num, NULL);
      line = pango_layout_get_line_readonly (layout, line_num);
      extra_chars = line->start_index + line->length - (blocks[i].text_length - prev_extra_chars);
      pango_layout_set_text (layout, blocks[i].text + prev_extra_chars,
	  blocks[i].text_length + extra_chars - prev_extra_chars);
    } else {
      pango_layout_set_text (layout, blocks[i].text + extra_chars, blocks[i].text_length - extra_chars);
      pango_layout_set_attributes (layout, blocks[i].attrs);
      extra_chars = 0;
    }

    if (fill)
      pango_cairo_show_layout (cr, layout);
    else
      pango_cairo_layout_path (cr, layout);

    pango_layout_get_pixel_size (layout, NULL, &height);
    cairo_rel_move_to (cr, -(blocks[i].left_margin + blocks[i].block_indent),
	height);
  }

  g_object_unref (layout);
}

int
tag_func_define_edit_text (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecEditText *text;
  guint id;
  int reserved;
  gboolean has_font, has_color, has_max_length, has_layout, has_text;
  SwfdecBits *b = &s->b;

  id = swfdec_bits_get_u16 (b);
  SWFDEC_LOG ("  id = %u", id);
  text = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_EDIT_TEXT);
  if (text == NULL)
    return SWFDEC_STATUS_OK;

  swfdec_bits_get_rect (b, &SWFDEC_GRAPHIC (text)->extents);
  SWFDEC_LOG ("  extents: %g %g  %g %g", 
      SWFDEC_GRAPHIC (text)->extents.x0, SWFDEC_GRAPHIC (text)->extents.y0,
      SWFDEC_GRAPHIC (text)->extents.x1, SWFDEC_GRAPHIC (text)->extents.y1);
  swfdec_bits_syncbits (b);
  has_text = swfdec_bits_getbit (b);
  text->wrap = swfdec_bits_getbit (b);
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
  text->border = swfdec_bits_getbit (b);
  reserved = swfdec_bits_getbit (b);
  text->html = swfdec_bits_getbit (b);
  text->embed_fonts = swfdec_bits_getbit (b);
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
