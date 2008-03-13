/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_shape_parser.h"
#include "swfdec_swf_decoder.h"
#include "swfdec_tag.h"

G_DEFINE_TYPE (SwfdecFont, swfdec_font, SWFDEC_TYPE_CHARACTER)

static void
swfdec_font_dispose (GObject *object)
{
  SwfdecFont * font = SWFDEC_FONT (object);
  guint i;

  if (font->glyphs) {
    for (i = 0; i < font->glyphs->len; i++) {
      SwfdecDraw *draw = g_array_index (font->glyphs, SwfdecFontEntry, i).draw;
      if (draw)
	g_object_unref (draw);
    }
    g_array_free (font->glyphs, TRUE);
    font->glyphs = NULL;
  }
  if (font->desc) {
    pango_font_description_free (font->desc);
    font->desc = NULL;
  }
  if (font->name) {
    g_free (font->name);
    font->name = NULL;
  }

  G_OBJECT_CLASS (swfdec_font_parent_class)->dispose (object);
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

/**
 * swfdec_font_get_glyph:
 * @font: a #SwfdecFont
 * @glyph: id of glyph to render
 *
 * Tries to get the shape associated with the given glyph id. It is valid to 
 * call this function with any glyph id. If no such glyph exists, this function 
 * returns %NULL.
 *
 * Returns: the shape of the requested glyph or %NULL if no such glyph exists.
 **/
SwfdecDraw *
swfdec_font_get_glyph (SwfdecFont * font, guint glyph)
{
  g_return_val_if_fail (SWFDEC_IS_FONT (font), NULL);
  
  if (glyph >= font->glyphs->len)
    return NULL;

  return g_array_index (font->glyphs, SwfdecFontEntry, glyph).draw;
}

#if 0
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
#endif

int
tag_func_define_font_info (SwfdecSwfDecoder *s, guint tag)
{
  SwfdecFont *font;
  guint id, len, i;
  int reserved, wide, ansi, jis;
  char *name;
  /* we just assume Latin1 (FIXME: option to change this?) */
  SwfdecLanguage language = SWFDEC_LANGUAGE_LATIN;

  id = swfdec_bits_get_u16 (&s->b);
  font = swfdec_swf_decoder_get_character (s, id);
  if (!SWFDEC_IS_FONT (font)) {
    SWFDEC_WARNING ("didn't find a font with id %u", id);
    return SWFDEC_STATUS_OK;
  }
  len = swfdec_bits_get_u8 (&s->b);
  /* this string is locale specific */
  name = swfdec_bits_get_string_length (&s->b, len, s->version);
  reserved = swfdec_bits_getbits (&s->b, 2);
  font->small = swfdec_bits_getbit (&s->b);
  jis = swfdec_bits_getbit (&s->b);
  ansi = swfdec_bits_getbit (&s->b);
  if (jis != 0 || ansi != 0) {
    SWFDEC_LOG ("ansi = %d, jis = %d", ansi, jis);
    if (tag == SWFDEC_TAG_DEFINEFONTINFO2)
      SWFDEC_INFO ("ANSI and JIS flags are supposed to be 0 in DefineFontInfo");
    if (jis)
      language = SWFDEC_LANGUAGE_JAPANESE;
  }
  font->italic = swfdec_bits_getbit (&s->b);
  font->bold = swfdec_bits_getbit (&s->b);
  wide = swfdec_bits_getbit (&s->b);
  if (tag == SWFDEC_TAG_DEFINEFONTINFO2)
    language = swfdec_bits_get_u8 (&s->b);
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

static void
swfdec_font_parse_shape (SwfdecSwfDecoder *s, SwfdecFontEntry *entry, guint size)
{
  SwfdecBits bits;
  SwfdecShapeParser *parser;
  GSList *list;

  swfdec_bits_init_bits (&bits, &s->b, size);
  parser = swfdec_shape_parser_new (NULL, NULL, NULL);
  swfdec_shape_parser_parse (parser, &bits);
  list = swfdec_shape_parser_free (parser);
  if (list) {
    entry->draw = g_object_ref (list->data);
    g_slist_foreach (list, (GFunc) g_object_unref, NULL);
    g_slist_free (list);
  } else {
    entry->draw = NULL;
  }

  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("parsing shape didn't use %d bytes",
	swfdec_bits_left (&bits) / 8);
  }
}

int
tag_func_define_font (SwfdecSwfDecoder * s, guint tag)
{
  guint i, id, n_glyphs, offset, next_offset;
  SwfdecFont *font;
  SwfdecBits offsets;

  id = swfdec_bits_get_u16 (&s->b);
  font = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_FONT);
  if (!font)
    return SWFDEC_STATUS_OK;
  font->scale_factor = SWFDEC_TEXT_SCALE_FACTOR;

  offset = swfdec_bits_get_u16 (&s->b);
  if (offset % 2) {
    SWFDEC_ERROR ("first offset is odd?!");
  }
  n_glyphs = offset / 2;
  if (n_glyphs == 0)
    return SWFDEC_STATUS_OK;
  swfdec_bits_init_bits (&offsets, &s->b, offset - 2);

  g_array_set_size (font->glyphs, n_glyphs);
  for (i = 0; i < n_glyphs && swfdec_bits_left (&s->b); i++) {
    SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
    if (i + 1 == n_glyphs)
      next_offset = offset + swfdec_bits_left (&s->b) / 8;
    else
      next_offset = swfdec_bits_get_u16 (&offsets);
    swfdec_font_parse_shape (s, entry, next_offset - offset);
    offset = next_offset;
  }
  if (i < n_glyphs) {
    SWFDEC_ERROR ("data was only enough for %u glyphs, not %u", i, n_glyphs);
    g_array_set_size (font->glyphs, i);
  }

  return SWFDEC_STATUS_OK;
}

static void
swfdec_font_parse_kerning_table (SwfdecSwfDecoder *s, SwfdecFont *font, gboolean wide_codes)
{
  SwfdecBits *bits = &s->b;
  guint n_kernings, i;

  n_kernings = swfdec_bits_get_u16 (bits);
  for (i = 0; i < n_kernings; i++) {
    if (wide_codes) {
      swfdec_bits_get_u16 (bits);
      swfdec_bits_get_u16 (bits);
    } else {
      swfdec_bits_get_u8 (bits);
      swfdec_bits_get_u8 (bits);
    }
    swfdec_bits_get_s16 (bits);
  }
}

int
tag_func_define_font_2 (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecBits offsets, *bits = &s->b;
  SwfdecFont *font;
  SwfdecLanguage language;
  guint i, id, len, n_glyphs, offset, next_offset;
  gboolean layout, shift_jis, ansi, wide_offsets, wide_codes;

  id = swfdec_bits_get_u16 (bits);
  font = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_FONT);
  if (!font)
    return SWFDEC_STATUS_OK;
  SWFDEC_LOG ("  id = %u", id);
  font->scale_factor = 20 * SWFDEC_TEXT_SCALE_FACTOR * (tag == SWFDEC_TAG_DEFINEFONT3 ? 20 : 1);

  layout = swfdec_bits_getbit (bits);
  SWFDEC_LOG (" layout = %d", layout);
  shift_jis = swfdec_bits_getbit (bits);
  SWFDEC_LOG (" JIS = %d", shift_jis);
  font->small = swfdec_bits_getbit (bits);
  SWFDEC_LOG (" small = %d", font->small);
  ansi = swfdec_bits_getbit (bits);
  SWFDEC_LOG (" ansi = %d", ansi);
  wide_offsets = swfdec_bits_getbit (bits);
  SWFDEC_LOG (" wide offsets = %d", wide_offsets);
  wide_codes = swfdec_bits_getbit (bits);
  SWFDEC_LOG (" wide codes = %d", wide_codes);
  if (wide_codes == 0) {
    SWFDEC_ERROR (" wide codes should be set in DefineFont3");
  }
  font->italic = swfdec_bits_getbit (bits);
  SWFDEC_LOG (" italic = %d", font->italic);
  font->bold = swfdec_bits_getbit (bits);
  SWFDEC_LOG (" bold = %d", font->bold);
  language = swfdec_bits_get_u8 (&s->b);
  SWFDEC_LOG (" language = %u", (guint) language);
  len = swfdec_bits_get_u8 (&s->b);
  font->name = swfdec_bits_get_string_length (&s->b, len, s->version);
  if (font->name == NULL) {
    SWFDEC_ERROR ("error reading font name");
  } else {
    SWFDEC_LOG ("  font name = %s", font->name);
  }
  n_glyphs = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG (" n_glyphs = %u", n_glyphs);
  
  if (wide_offsets) {
    offset = swfdec_bits_get_u32 (bits);
    swfdec_bits_init_bits (&offsets, bits, n_glyphs * 4);
  } else {
    offset = swfdec_bits_get_u16 (bits);
    swfdec_bits_init_bits (&offsets, bits, n_glyphs * 2);
  }
  g_array_set_size (font->glyphs, n_glyphs);
  for (i = 0; i < n_glyphs && swfdec_bits_left (bits); i++) {
    SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
    if (wide_offsets)
      next_offset = swfdec_bits_get_u32 (&offsets);
    else
      next_offset = swfdec_bits_get_u16 (&offsets);
    swfdec_font_parse_shape (s, entry, next_offset - offset);
    offset = next_offset;
  }
  if (i < n_glyphs) {
    SWFDEC_ERROR ("data was only enough for %u glyphs, not %u", i, n_glyphs);
    g_array_set_size (font->glyphs, i);
    n_glyphs = i;
  }
  for (i = 0; i < n_glyphs && swfdec_bits_left (bits); i++) {
    SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
    if (wide_codes)
      entry->value = swfdec_bits_get_u16 (bits);
    else
      entry->value = swfdec_bits_get_u8 (bits);
  }
  if (layout) {
    font->ascent = swfdec_bits_get_u16 (bits);
    font->descent = swfdec_bits_get_u16 (bits);
    font->leading = swfdec_bits_get_u16 (bits);
    for (i = 0; i < n_glyphs && swfdec_bits_left (bits); i++) {
      SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
      entry->advance = swfdec_bits_get_u16 (bits);
    }
    for (i = 0; i < n_glyphs && swfdec_bits_left (bits); i++) {
      SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
      swfdec_bits_get_rect (bits, &entry->extents);
    }
    swfdec_font_parse_kerning_table (s, font, wide_codes);
  } else {
    font->ascent = swfdec_bits_get_u16 (bits);
    font->descent = swfdec_bits_get_u16 (bits);
    font->leading = swfdec_bits_get_u16 (bits);
    for (i = 0; i < n_glyphs && swfdec_bits_left (bits); i++) {
      SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
      entry->advance = font->scale_factor;
      entry->extents.x0 = entry->extents.y0 = 0;
      entry->extents.x1 = entry->extents.y1 = font->scale_factor;
    }
  }

  return SWFDEC_STATUS_OK;
}
