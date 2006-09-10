

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

}

static void
swfdec_font_dispose (SwfdecFont * font)
{
  SwfdecShape *shape;
  unsigned int i;

  for (i = 0; i < font->glyphs->len; i++) {
    shape = g_ptr_array_index (font->glyphs, i);
    swfdec_object_unref (SWFDEC_OBJECT (shape));
  }
  g_ptr_array_free (font->glyphs, TRUE);
}

SwfdecShape *
swfdec_font_get_glyph (SwfdecFont * font, unsigned int glyph)
{
  g_return_val_if_fail (SWFDEC_IS_FONT (font), NULL);
  g_return_val_if_fail (glyph < font->glyphs->len, NULL);

  return g_ptr_array_index (font->glyphs, glyph);
}


