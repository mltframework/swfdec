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
#include "swfdec_shape.h"
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
SwfdecShape *
swfdec_font_get_glyph (SwfdecFont * font, unsigned int glyph)
{
  g_return_val_if_fail (SWFDEC_IS_FONT (font), NULL);
  
  if (glyph >= font->glyphs->len)
    return NULL;

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

static void
swfdec_font_parse_shape (SwfdecSwfDecoder *s, SwfdecFontEntry *entry, guint size)
{
  SwfdecBits save_bits = s->b;
  SwfdecShape *shape = g_object_new (SWFDEC_TYPE_SHAPE, NULL);
  entry->shape = shape;

  g_ptr_array_add (shape->fills, swfdec_pattern_new_color (0xFFFFFFFF));
  g_ptr_array_add (shape->lines, swfdec_pattern_new_stroke (20, 0xFFFFFFFF));

  shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
  SWFDEC_LOG ("n_fill_bits = %d", shape->n_fill_bits);
  shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
  SWFDEC_LOG ("n_line_bits = %d", shape->n_line_bits);

  swfdec_shape_get_recs (s, shape);
  swfdec_bits_syncbits (&s->b);
  if (swfdec_bits_skip_bytes (&save_bits, size) != size) {
    SWFDEC_ERROR ("invalid offset value, not enough bytes available");
  }
  if (swfdec_bits_left (&save_bits) != swfdec_bits_left (&s->b)) {
    SWFDEC_WARNING ("parsing shape did use %d bytes too much\n",
	(swfdec_bits_left (&save_bits) - swfdec_bits_left (&s->b)) / 8);
    /* we trust the offsets here */
    s->b = save_bits;
  }
}

int
tag_func_define_font (SwfdecSwfDecoder * s)
{
  unsigned int i, id, n_glyphs, offset, next_offset;
  SwfdecFont *font;
  SwfdecBits offsets;

  id = swfdec_bits_get_u16 (&s->b);
  font = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_FONT);
  if (!font)
    return SWFDEC_STATUS_OK;

  offsets = s->b;
  n_glyphs = swfdec_bits_get_u16 (&s->b);
  if (n_glyphs % 2) {
    SWFDEC_ERROR ("first offset is odd?!");
  }
  n_glyphs /= 2;
  if (swfdec_bits_skip_bytes (&s->b, n_glyphs * 2 - 2) != n_glyphs * 2 - 2) {
    SWFDEC_ERROR ("invalid glyph offsets");
    return SWFDEC_STATUS_OK;
  }

  g_array_set_size (font->glyphs, n_glyphs);
  offset = swfdec_bits_get_u16 (&offsets);
  for (i = 0; i < n_glyphs; i++) {
    SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
    if (i + 1 == n_glyphs)
      next_offset = offset + swfdec_bits_left (&s->b) / 8;
    else
      next_offset = swfdec_bits_get_u16 (&offsets);
    swfdec_font_parse_shape (s, entry, next_offset - offset);
    offset = next_offset;
  }

  return SWFDEC_STATUS_OK;
}

static void
get_kerning_record (SwfdecBits * bits, int wide_codes)
{
  if (wide_codes) {
    swfdec_bits_get_u16 (bits);
    swfdec_bits_get_u16 (bits);
  } else {
    swfdec_bits_get_u8 (bits);
    swfdec_bits_get_u8 (bits);
  }
  swfdec_bits_get_s16 (bits);
}

int
tag_func_define_font_2 (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  unsigned int id;
  SwfdecShape *shape;
  SwfdecFont *font;
  SwfdecRect rect;

  int has_layout;
  int shift_jis;
  int reserved;
  int ansi;
  int wide_offsets;
  int wide_codes;
  int italic;
  int bold;
  int langcode;
  int font_name_len;
  int n_glyphs;
  int code_table_offset;
  int font_ascent;
  int font_descent;
  int font_leading;
  int kerning_count;
  int i;

  id = swfdec_bits_get_u16 (bits);
  font = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_FONT);
  if (!font)
    return SWFDEC_STATUS_OK;

  has_layout = swfdec_bits_getbit (bits);
  shift_jis = swfdec_bits_getbit (bits);
  reserved = swfdec_bits_getbit (bits);
  ansi = swfdec_bits_getbit (bits);
  wide_offsets = swfdec_bits_getbit (bits);
  wide_codes = swfdec_bits_getbit (bits);
  italic = swfdec_bits_getbit (bits);
  bold = swfdec_bits_getbit (bits);

  langcode = swfdec_bits_get_u8 (bits);
  SWFDEC_DEBUG("langcode %d", langcode);

  font_name_len = swfdec_bits_get_u8 (bits);
  //font_name = 
  bits->ptr += font_name_len;

  n_glyphs = swfdec_bits_get_u16 (bits);
  if (wide_offsets) {
    bits->ptr += 4 * n_glyphs;
    code_table_offset = swfdec_bits_get_u32 (bits);
  } else {
    bits->ptr += 2 * n_glyphs;
    code_table_offset = swfdec_bits_get_u16 (bits);
  }

  g_array_set_size (font->glyphs, n_glyphs);

  for (i = 0; i < n_glyphs; i++) {
    SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
    shape = g_object_new (SWFDEC_TYPE_SHAPE, NULL);
    entry->shape = shape;

    g_ptr_array_add (shape->fills, swfdec_pattern_new_color (0xFFFFFFFF));
    g_ptr_array_add (shape->lines, swfdec_pattern_new_stroke (20, 0xFFFFFFFF));

    swfdec_bits_syncbits (&s->b);
    shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_fill_bits = %d", shape->n_fill_bits);
    shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_line_bits = %d", shape->n_line_bits);

    swfdec_shape_get_recs (s, shape);
  }
  if (wide_codes) {
    swfdec_bits_skip_bytes (bits, 2 * n_glyphs);
  } else {
    swfdec_bits_skip_bytes (bits, 1 * n_glyphs);
  }
  if (has_layout) {
    font_ascent = swfdec_bits_get_s16 (bits);
    font_descent = swfdec_bits_get_s16 (bits);
    font_leading = swfdec_bits_get_s16 (bits);
    //font_advance_table = swfdec_bits_get_s16(bits);
    swfdec_bits_skip_bytes (bits, 2 * n_glyphs);
    for (i = 0; i < n_glyphs; i++) {
      swfdec_bits_get_rect (bits, &rect);
    }
    kerning_count = swfdec_bits_get_u16 (bits);
    if (0) {
      for (i = 0; i < kerning_count; i++) {
        get_kerning_record (bits, wide_codes);
      }
    }
  }

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_font_3 (SwfdecSwfDecoder * s)
{
  return SWFDEC_STATUS_OK;
}
