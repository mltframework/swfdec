

#include <swfdec_font.h>
#include <swfdec_internal.h>


SWFDEC_OBJECT_BOILERPLATE (SwfdecFont, swfdec_font)


     static void swfdec_font_base_init (gpointer g_class)
{

}

static void
swfdec_font_class_init (SwfdecFontClass * g_class)
{

}

static void
swfdec_font_init (SwfdecFont * font)
{
  font->glyphs = g_array_new (FALSE, TRUE, sizeof (SwfdecFontEntry));
}

static void
swfdec_font_dispose (SwfdecFont * font)
{
  guint i;

  for (i = 0; i < font->glyphs->len; i++) {
    swfdec_object_unref (g_array_index (font->glyphs, SwfdecFontEntry, i).shape);
  }

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (font));
}

SwfdecShape *
swfdec_font_get_glyph (SwfdecFont * font, unsigned int glyph)
{
  g_return_val_if_fail (SWFDEC_IS_FONT (font), NULL);
  g_return_val_if_fail (glyph < font->glyphs->len, NULL);

  return g_array_index (font->glyphs, SwfdecFontEntry, glyph).shape;
}

char *
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
tag_func_define_font_info (SwfdecDecoder *s, unsigned int version)
{
  SwfdecFont *font;
  unsigned int id, len, i;
  int reserved, wide, ansi, jis;
  char *name;
  /* we just assume Latin1 (FIXME: option to change this?) */
  SwfdecLanguage language = SWFDEC_LANGUAGE_LATIN;

  g_assert (version == 1 || version == 2);

  id = swfdec_bits_get_u16 (&s->b);
  font = swfdec_object_get (s, id);
  if (!SWFDEC_IS_FONT (font))
    return SWF_OK;
  len = swfdec_bits_get_u8 (&s->b);
  /* this string is locale specific */
  name = swfdec_bits_get_string_length (&s->b, len);
  reserved = swfdec_bits_getbits (&s->b, 2);
  font->small = swfdec_bits_getbit (&s->b);
  jis = swfdec_bits_getbit (&s->b);
  ansi = swfdec_bits_getbit (&s->b);
  if (jis != 0 || ansi != 0) {
    g_print ("ansi = %d, jis = %d\n", ansi, jis);
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
    /* FIXME: get a pango layout here */
  }
  for (i = 0; i < font->glyphs->len; i++) {
    g_array_index (font->glyphs, SwfdecFontEntry, i).value = 
      wide ? swfdec_bits_get_u16 (&s->b) : swfdec_bits_get_u8 (&s->b);
  }

  return SWF_OK;
}

