/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_font.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecFont, swfdec_font, SWFDEC_TYPE_CHARACTER)

static void
swfdec_font_dispose (GObject *object)
{
  SwfdecFont * font = SWFDEC_FONT (object);
  guint i;

  for (i = 0; i < font->glyphs->len; i++) {
    g_object_unref (g_array_index (font->glyphs, SwfdecFontEntry, i).shape);
  }
  if (font->desc)
    pango_font_description_free (font->desc);
  g_free (font->name);

  G_OBJECT_CLASS (swfdec_font_parent_class)->dispose (G_OBJECT (font));
}

static void
swfdec_font_class_init (SwfdecFontClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_font_dispose;
}

static void
swfdec_font_init (SwfdecFont * font)
{
  font->glyphs = g_array_new (FALSE, TRUE, sizeof (SwfdecFontEntry));
}

SwfdecShape *
swfdec_font_get_glyph (SwfdecFont * font, unsigned int glyph)
{
  g_return_val_if_fail (SWFDEC_IS_FONT (font), NULL);
  g_return_val_if_fail (glyph < font->glyphs->len, NULL);

  return g_array_index (font->glyphs, SwfdecFontEntry, glyph).shape;
}

static char *
convert_from_language (const char *s, SwfdecLanguage language)
{
  char *ret;
  const char *langcode;

  switch (language) {
    case SWFDEC_LANGUAGE_LATIN:
      langcode = "LATIN1";
      break;
    case SWFDEC_LANGUAGE_JAPANESE:
      langcode = "SHIFT_JIS";
      break;
      /* FIXME! */
    case SWFDEC_LANGUAGE_KOREAN:
    case SWFDEC_LANGUAGE_CHINESE:
    case SWFDEC_LANGUAGE_CHINESE_TRADITIONAL:
    default:
      SWFDEC_ERROR ("can't convert given text from language %u", language);
      return NULL;
  }
  SWFDEC_LOG ("converting text from %s", langcode);
  ret = g_convert (s, -1, "UTF-8", langcode, NULL, NULL, NULL);
  if (ret == NULL)
    SWFDEC_ERROR ("given text is not in language %s", langcode);
  return ret;
}

int
tag_func_define_font_info (SwfdecSwfDecoder *s, unsigned int version)
{
  SwfdecFont *font;
  unsigned int id, len, i;
  int reserved, wide, ansi, jis;
  char *name;
  /* we just assume Latin1 (FIXME: option to change this?) */
  SwfdecLanguage language = SWFDEC_LANGUAGE_LATIN;

  g_assert (version == 1 || version == 2);

  id = swfdec_bits_get_u16 (&s->b);
  font = swfdec_swf_decoder_get_character (s, id);
  if (!SWFDEC_IS_FONT (font)) {
    SWFDEC_WARNING ("didn't find a font with id %u", id);
    return SWFDEC_STATUS_OK;
  }
  len = swfdec_bits_get_u8 (&s->b);
  /* this string is locale specific */
  name = swfdec_bits_get_string_length (&s->b, len);
  reserved = swfdec_bits_getbits (&s->b, 2);
  font->small = swfdec_bits_getbit (&s->b);
  jis = swfdec_bits_getbit (&s->b);
  ansi = swfdec_bits_getbit (&s->b);
  if (jis != 0 || ansi != 0) {
    SWFDEC_LOG ("ansi = %d, jis = %d", ansi, jis);
    if (version == 2)
      SWFDEC_INFO ("ANSI and JIS flags are supposed to be 0 in DefineFontInfo");
    if (jis)
      language = SWFDEC_LANGUAGE_JAPANESE;
  }
  font->italic = swfdec_bits_getbit (&s->b);
  font->bold = swfdec_bits_getbit (&s->b);
  wide = swfdec_bits_getbit (&s->b);
  if (version > 1)
    language = swfdec_bits_get_u8 (&s->b);
  font->name = convert_from_language (name, language);
  g_free (name);
  if (font->name) {
    SWFDEC_LOG ("Creating font description for font %d", id);
    font->desc = pango_font_description_new ();
    pango_font_description_set_family_static (font->desc, font->name);
    if (font->bold)
      pango_font_description_set_weight (font->desc, PANGO_WEIGHT_BOLD);
    if (font->italic)
      pango_font_description_set_style (font->desc, PANGO_STYLE_ITALIC);
  }
  for (i = 0; i < font->glyphs->len; i++) {
    g_array_index (font->glyphs, SwfdecFontEntry, i).value = 
      wide ? swfdec_bits_get_u16 (&s->b) : swfdec_bits_get_u8 (&s->b);
  }

  return SWFDEC_STATUS_OK;
}

