
#include <swfdec_text.h>
#include "swfdec_internal.h"
#include <swfdec_font.h>
#include <swfdec_render.h>

static void
swfdec_color_transform_init_color (SwfdecColorTransform *trans, swf_color color)
{
  trans->mult[0] = 0.0;
  trans->mult[1] = 0.0;
  trans->mult[2] = 0.0;
  trans->mult[3] = 0.0;
  trans->add[0] = SWF_COLOR_R (color);
  trans->add[1] = SWF_COLOR_G (color);
  trans->add[2] = SWF_COLOR_B (color);
  trans->add[3] = SWF_COLOR_A (color);
}

static void
swfdec_text_render (SwfdecDecoder *s, cairo_t *cr, 
    const SwfdecColorTransform *trans, SwfdecObject *obj, SwfdecRect *inval)
{
  unsigned int i, color;
  SwfdecText *text = SWFDEC_TEXT (obj);
  SwfdecObject *fontobj;
  SwfdecColorTransform force_color;
  int x = 0;

  for (i = 0; i < text->glyphs->len; i++) {
    SwfdecTextGlyph *glyph;
    SwfdecShape *shape;
    cairo_matrix_t pos;

    glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);

    fontobj = swfdec_object_get (s, glyph->font);
    if (fontobj == NULL)
      continue;

    shape = swfdec_font_get_glyph (SWFDEC_FONT (fontobj), glyph->glyph);
    if (shape == NULL) {
      SWFDEC_ERROR ("failed getting glyph %d\n", glyph->glyph);
      continue;
    }

    cairo_matrix_init_translate (&pos,
	glyph->x, glyph->y * SWF_SCALE_FACTOR);
    pos.xx = glyph->height * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR;
    pos.yy = glyph->height * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR;
    color = swfdec_color_apply_transform (glyph->color, trans);
    swfdec_color_transform_init_color (&force_color, color);
    swfdec_object_render (s, SWFDEC_OBJECT (shape), cr, &pos,
	&force_color, inval);
    x += glyph->x;
  }
}

SWFDEC_OBJECT_BOILERPLATE (SwfdecText, swfdec_text)

     static void swfdec_text_base_init (gpointer g_class)
{

}

static void
swfdec_text_class_init (SwfdecTextClass * g_class)
{
  SWFDEC_OBJECT_CLASS (g_class)->render = swfdec_text_render;
}

static void
swfdec_text_init (SwfdecText * text)
{
  text->glyphs = g_array_new (FALSE, TRUE, sizeof (SwfdecTextGlyph));
}

static void
swfdec_text_dispose (SwfdecText * text)
{
  g_array_free (text->glyphs, TRUE);
}

