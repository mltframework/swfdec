

#include <swfdec_font.h>
#include <swfdec_internal.h>


static void swfdec_font_base_init (gpointer g_class);
static void swfdec_font_class_init (gpointer g_class, gpointer user_data);
static void swfdec_font_init (GTypeInstance *instance, gpointer g_class);
static void swfdec_font_dispose (GObject *object);


GType _swfdec_font_type;

static GObjectClass *parent_class = NULL;

GType swfdec_font_get_type (void)
{
  if (!_swfdec_font_type) {
    static const GTypeInfo object_info = {
      sizeof (SwfdecFontClass),
      swfdec_font_base_init,
      NULL,
      swfdec_font_class_init,
      NULL,
      NULL,
      sizeof (SwfdecFont),
      32,
      swfdec_font_init,
      NULL
    };
    _swfdec_font_type = g_type_register_static (SWFDEC_TYPE_OBJECT,
        "SwfdecFont", &object_info, 0);
  }
  return _swfdec_font_type;
}

static void swfdec_font_base_init (gpointer g_class)
{
  //GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

}

static void swfdec_font_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  gobject_class->dispose = swfdec_font_dispose;

  parent_class = g_type_class_peek_parent (gobject_class);

}

static void swfdec_font_init (GTypeInstance *instance, gpointer g_class)
{

}

static void swfdec_font_dispose (GObject *object)
{
  SwfdecFont *font = SWFDEC_FONT (object);
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

  id = get_u16 (&s->b);
  font = g_object_new (SWFDEC_TYPE_FONT, NULL);
  SWFDEC_OBJECT (font)-> id = id;

  offset = get_u16 (&s->b);
  n_glyphs = offset / 2;

  for (i = 1; i < n_glyphs; i++) {
    offset = get_u16 (&s->b);
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
    syncbits (&s->b);
    shape->n_fill_bits = getbits (&s->b, 4);
    SWF_DEBUG (0, "n_fill_bits = %d\n", shape->n_fill_bits);
    shape->n_line_bits = getbits (&s->b, 4);
    SWF_DEBUG (0, "n_line_bits = %d\n", shape->n_line_bits);

    swf_shape_get_recs (s, &s->b, shape);
  }

  return SWF_OK;
}

static void
get_kerning_record (bits_t * bits, int wide_codes)
{
  if (wide_codes) {
    get_u16 (bits);
    get_u16 (bits);
  } else {
    get_u8 (bits);
    get_u8 (bits);
  }
  get_s16 (bits);
}

int
tag_func_define_font_2 (SwfdecDecoder * s)
{
  bits_t *bits = &s->b;
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

  id = get_u16 (bits);
  font = g_object_new (SWFDEC_TYPE_FONT, NULL);
  SWFDEC_OBJECT (font)->id = id;

  has_layout = getbit (bits);
  shift_jis = getbit (bits);
  reserved = getbit (bits);
  ansi = getbit (bits);
  wide_offsets = getbit (bits);
  wide_codes = getbit (bits);
  italic = getbit (bits);
  bold = getbit (bits);

  langcode = get_u8 (bits);

  font_name_len = get_u8 (bits);
  //font_name = 
  bits->ptr += font_name_len;

  n_glyphs = get_u16 (bits);
  if (wide_offsets) {
    bits->ptr += 4 * n_glyphs;
    code_table_offset = get_u32 (bits);
  } else {
    bits->ptr += 2 * n_glyphs;
    code_table_offset = get_u16 (bits);
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
    syncbits (&s->b);
    shape->n_fill_bits = getbits (&s->b, 4);
    SWF_DEBUG (0, "n_fill_bits = %d\n", shape->n_fill_bits);
    shape->n_line_bits = getbits (&s->b, 4);
    SWF_DEBUG (0, "n_line_bits = %d\n", shape->n_line_bits);

    swf_shape_get_recs (s, &s->b, shape);
  }
  if (wide_codes) {
    bits->ptr += 2 * n_glyphs;
  } else {
    bits->ptr += 1 * n_glyphs;
  }
  if (has_layout) {
    font_ascent = get_s16 (bits);
    font_descent = get_s16 (bits);
    font_leading = get_s16 (bits);
    //font_advance_table = get_s16(bits);
    bits->ptr += 2 * n_glyphs;
    //font_bounds = get_s16(bits);
    for (i = 0; i < n_glyphs; i++) {
      get_rect (bits, rect);
    }
    kerning_count = get_u16 (bits);
    for (i = 0; i < kerning_count; i++) {
      get_kerning_record (bits, wide_codes);
    }
  }

  return SWF_OK;
}

