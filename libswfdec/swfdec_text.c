
#include <swfdec_text.h>
#include "swfdec_internal.h"
#include <swfdec_font.h>


static void swfdec_text_base_init (gpointer g_class);
static void swfdec_text_class_init (gpointer g_class, gpointer user_data);
static void swfdec_text_init (GTypeInstance *instance, gpointer g_class);
static void swfdec_text_dispose (GObject *object);


GType _swfdec_text_type;

static GObjectClass *parent_class = NULL;

GType swfdec_text_get_type (void)
{
  if (!_swfdec_text_type) {
    static const GTypeInfo object_info = {
      sizeof (SwfdecTextClass),
      swfdec_text_base_init,
      NULL,
      swfdec_text_class_init,
      NULL,
      NULL,
      sizeof (SwfdecText),
      0,
      swfdec_text_init,
      NULL
    };
    _swfdec_text_type = g_type_register_static (SWFDEC_TYPE_OBJECT,
        "SwfdecText", &object_info, 0);
  }
  return _swfdec_text_type;
}

static void swfdec_text_base_init (gpointer g_class)
{
  //GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

}

static void swfdec_text_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  gobject_class->dispose = swfdec_text_dispose;

  parent_class = g_type_class_peek_parent (gobject_class);

  SWFDEC_OBJECT_CLASS (g_class)->prerender = swfdec_text_prerender;
}

static void swfdec_text_init (GTypeInstance *instance, gpointer g_class)
{
  SwfdecText *text = SWFDEC_TEXT (instance);

  text->glyphs = g_array_new (FALSE, TRUE, sizeof (SwfdecTextGlyph));
}

static void swfdec_text_dispose (GObject *object)
{
  SwfdecText *text = SWFDEC_TEXT (object);

  g_array_free (text->glyphs, TRUE);
}


static int
define_text (SwfdecDecoder * s, int rgba)
{
  SwfdecBits *bits = &s->b;
  int id;
  int rect[4];
  int n_glyph_bits;
  int n_advance_bits;
  SwfdecText *text = NULL;
  SwfdecTextGlyph glyph = { 0 };

  id = swfdec_bits_get_u16 (bits);
  text = g_object_new (SWFDEC_TYPE_TEXT, NULL);
  SWFDEC_OBJECT (text)->id = id;
  s->objects = g_list_append (s->objects, text);

  glyph.color = 0xffffffff;

  swfdec_bits_get_rect (bits, rect);
  swfdec_bits_get_transform (bits, &SWFDEC_OBJECT (text)->transform);
  swfdec_bits_syncbits (bits);
  n_glyph_bits = swfdec_bits_get_u8 (bits);
  n_advance_bits = swfdec_bits_get_u8 (bits);

  //printf("  id = %d\n", id);
  //printf("  n_glyph_bits = %d\n", n_glyph_bits);
  //printf("  n_advance_bits = %d\n", n_advance_bits);

  while (swfdec_bits_peekbits (bits, 8) != 0) {
    int type;

    type = swfdec_bits_getbit (bits);
    if (type == 0) {
      /* glyph record */
      int n_glyphs;
      int i;

      n_glyphs = swfdec_bits_getbits (bits, 7);
      for (i = 0; i < n_glyphs; i++) {
	glyph.glyph = swfdec_bits_getbits (bits, n_glyph_bits);

	g_array_append_val (text->glyphs, glyph);
	glyph.x += swfdec_bits_getbits (bits, n_advance_bits);
      }
    } else {
      /* state change */
      int reserved;
      int has_font;
      int has_color;
      int has_y_offset;
      int has_x_offset;

      reserved = swfdec_bits_getbits (bits, 3);
      has_font = swfdec_bits_getbit (bits);
      has_color = swfdec_bits_getbit (bits);
      has_y_offset = swfdec_bits_getbit (bits);
      has_x_offset = swfdec_bits_getbit (bits);
      if (has_font) {
	glyph.font = swfdec_bits_get_u16 (bits);
	//printf("  font = %d\n",font);
      }
      if (has_color) {
	if (rgba) {
	  glyph.color = swfdec_bits_get_rgba (bits);
	} else {
	  glyph.color = swfdec_bits_get_color (bits);
	}
	//printf("  color = %08x\n",glyph.color);
      }
      if (has_x_offset) {
	glyph.x = swfdec_bits_get_u16 (bits);
	//printf("  x = %d\n",x);
      }
      if (has_y_offset) {
	glyph.y = swfdec_bits_get_u16 (bits);
	//printf("  y = %d\n",y);
      }
      if (has_font) {
	glyph.height = swfdec_bits_get_u16 (bits);
	//printf("  height = %d\n",height);
      }
    }
    swfdec_bits_syncbits (bits);
  }
  swfdec_bits_get_u8 (bits);

  return SWF_OK;
}

int
tag_func_define_text (SwfdecDecoder * s)
{
  return define_text (s, 0);
}

int
tag_func_define_text_2 (SwfdecDecoder * s)
{
  return define_text (s, 1);
}


