

#include <swfdec_font.h>
#include <swfdec_internal.h>


SWFDEC_OBJECT_BOILERPLATE (SwfdecFont, swfdec_font)


static void swfdec_font_base_init (gpointer g_class)
{

}

static void swfdec_font_class_init (SwfdecFontClass *g_class)
{

}

static void swfdec_font_init (SwfdecFont *font)
{

}

static void swfdec_font_dispose (SwfdecFont *font)
{
  SwfdecShape *shape;
  int i;

  for (i = 0; i < font->glyphs->len; i++) {
    shape = g_ptr_array_index (font->glyphs, i);
    g_object_unref (G_OBJECT (shape));
  }
  g_ptr_array_free (font->glyphs, TRUE);
}

SwfdecShape *swfdec_font_get_glyph (SwfdecFont *font, int glyph)
{
  g_return_val_if_fail (SWFDEC_IS_FONT (font), NULL);
  g_return_val_if_fail (glyph >= 0 && glyph < font->glyphs->len, NULL);

  return g_ptr_array_index (font->glyphs, glyph);
}


int
tag_func_define_font (SwfdecDecoder * s)
{
  int id;
  int i;
  int n_glyphs;
  int offset;
  SwfdecShapeVec *shapevec;
  SwfdecShape *shape;
  SwfdecFont *font;

  id = swfdec_bits_get_u16 (&s->b);
  font = g_object_new (SWFDEC_TYPE_FONT, NULL);
  SWFDEC_OBJECT (font)-> id = id;
  s->objects = g_list_append (s->objects, font);

  offset = swfdec_bits_get_u16 (&s->b);
  n_glyphs = offset / 2;

  for (i = 1; i < n_glyphs; i++) {
    offset = swfdec_bits_get_u16 (&s->b);
  }

  font->glyphs = g_ptr_array_sized_new (n_glyphs);

  for (i = 0; i < n_glyphs; i++) {
    shape = g_object_new (SWFDEC_TYPE_SHAPE, NULL);
    g_ptr_array_add (font->glyphs, shape);

    shape->fills = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills, shapevec);
    shape->fills2 = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills2, shapevec);
    shape->lines = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->lines, shapevec);

    //swf_shape_add_styles(s,shape,&s->b);
    swfdec_bits_syncbits (&s->b);
    shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG("n_fill_bits = %d", shape->n_fill_bits);
    shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG("n_line_bits = %d", shape->n_line_bits);

    swf_shape_get_recs (s, &s->b, shape);
  }

  return SWF_OK;
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
tag_func_define_font_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  SwfdecShapeVec *shapevec;
  SwfdecShape *shape;
  SwfdecFont *font;
  int rect[4];

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
  font = g_object_new (SWFDEC_TYPE_FONT, NULL);
  SWFDEC_OBJECT (font)->id = id;
  s->objects = g_list_append (s->objects, font);

  has_layout = swfdec_bits_getbit (bits);
  shift_jis = swfdec_bits_getbit (bits);
  reserved = swfdec_bits_getbit (bits);
  ansi = swfdec_bits_getbit (bits);
  wide_offsets = swfdec_bits_getbit (bits);
  wide_codes = swfdec_bits_getbit (bits);
  italic = swfdec_bits_getbit (bits);
  bold = swfdec_bits_getbit (bits);

  langcode = swfdec_bits_get_u8 (bits);

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

  font->glyphs = g_ptr_array_sized_new (n_glyphs);

  for (i = 0; i < n_glyphs; i++) {
    shape = g_object_new (SWFDEC_TYPE_SHAPE, NULL);
    g_ptr_array_add (font->glyphs, shape);

    shape->fills = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills, shapevec);
    shape->fills2 = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills2, shapevec);
    shape->lines = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->lines, shapevec);

    //swf_shape_add_styles(s,shape,&s->b);
    swfdec_bits_syncbits (&s->b);
    shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG("n_fill_bits = %d", shape->n_fill_bits);
    shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG("n_line_bits = %d", shape->n_line_bits);

    swf_shape_get_recs (s, &s->b, shape);
  }
  if (wide_codes) {
    bits->ptr += 2 * n_glyphs;
  } else {
    bits->ptr += 1 * n_glyphs;
  }
  if (has_layout) {
    font_ascent = swfdec_bits_get_s16 (bits);
    font_descent = swfdec_bits_get_s16 (bits);
    font_leading = swfdec_bits_get_s16 (bits);
    //font_advance_table = swfdec_bits_get_s16(bits);
    bits->ptr += 2 * n_glyphs;
    //font_bounds = swfdec_bits_get_s16(bits);
    for (i = 0; i < n_glyphs; i++) {
      swfdec_bits_get_rect (bits, rect);
    }
    kerning_count = swfdec_bits_get_u16 (bits);
    for (i = 0; i < kerning_count; i++) {
      get_kerning_record (bits, wide_codes);
    }
  }

  return SWF_OK;
}

