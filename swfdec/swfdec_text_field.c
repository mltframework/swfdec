/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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
  SwfdecTextFieldMovie *ret =
    g_object_new (SWFDEC_TYPE_TEXT_FIELD_MOVIE, NULL);

  ret->html = text->html;
  ret->editable = text->editable;
  ret->password = text->password;
  ret->max_chars = text->max_chars;
  ret->selectable = text->selectable;
  ret->embed_fonts = text->embed_fonts;
  ret->word_wrap = text->word_wrap;
  ret->multiline = text->multiline;
  ret->auto_size = text->auto_size;
  ret->border = text->border;

  *size = sizeof (SwfdecTextFieldMovie);

  return SWFDEC_MOVIE (ret);
}

static void
swfdec_text_field_dispose (GObject *object)
{
  SwfdecTextField *text = SWFDEC_TEXT_FIELD (object);

  if (text->input != NULL) {
    g_free (text->input);
    text->input = NULL;
  }
  if (text->variable != NULL) {
    g_free (text->variable);
    text->variable = NULL;
  }
  if (text->font != NULL) {
    g_free (text->font);
    text->font = NULL;
  }

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
  text->editable = !swfdec_bits_getbit (b);
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
      text->font = g_strdup (SWFDEC_FONT (font)->name);
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
    text->color = SWFDEC_COLOR_COMBINE (255, 255, 255, 0);
  }

  if (has_max_length) {
    text->max_chars = swfdec_bits_get_u16 (b);
  } else {
    text->max_chars = 0;
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

  text->variable = swfdec_bits_get_string (b, s->version);
  if (text->variable && *text->variable == 0) {
    g_free (text->variable);
    text->variable = NULL;
  }

  if (has_text)
    text->input = swfdec_bits_get_string (b, s->version);

  return SWFDEC_STATUS_OK;
}
