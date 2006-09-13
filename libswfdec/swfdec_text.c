
#include <swfdec_text.h>
#include "swfdec_internal.h"
#include <swfdec_font.h>

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

static SwfdecMouseResult 
swfdec_text_handle_mouse (SwfdecDecoder *s, SwfdecObject *object,
      double x, double y, int button, SwfdecRect *inval)
{
  unsigned int i;
  SwfdecMouseResult ret;
  SwfdecObject *fontobj;
  SwfdecText *text = SWFDEC_TEXT (object);

  swfdec_matrix_transform_point_inverse (&text->transform, &x, &y);
  for (i = 0; i < text->glyphs->len; i++) {
    SwfdecTextGlyph *glyph;
    SwfdecShape *shape;

    glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);
    fontobj = swfdec_object_get (s, glyph->font);
    if (fontobj == NULL)
      continue;

    shape = swfdec_font_get_glyph (SWFDEC_FONT (fontobj), glyph->glyph);
    ret = swfdec_object_handle_mouse (s, SWFDEC_OBJECT (shape),
	x - glyph->x, y - glyph->y * SWF_SCALE_FACTOR, button, TRUE, inval);
    /* it's just shapes and shapes don't invalidate anything */
    if (ret != SWFDEC_MOUSE_MISSED)
      return ret;
  }
  return SWFDEC_MOUSE_MISSED;
}

static void
swfdec_text_render (SwfdecDecoder *s, cairo_t *cr, 
    const SwfdecColorTransform *trans, SwfdecObject *obj, SwfdecRect *inval)
{
  unsigned int i, color;
  SwfdecText *text = SWFDEC_TEXT (obj);
  SwfdecObject *fontobj;
  SwfdecColorTransform force_color;

  cairo_transform (cr, &text->transform);
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
  }
}

SWFDEC_OBJECT_BOILERPLATE (SwfdecText, swfdec_text)

     static void swfdec_text_base_init (gpointer g_class)
{

}

static void
swfdec_text_class_init (SwfdecTextClass * g_class)
{
  SwfdecObjectClass *object_class = SWFDEC_OBJECT_CLASS (g_class);

  object_class->render = swfdec_text_render;
  object_class->handle_mouse = swfdec_text_handle_mouse;
}

static void
swfdec_text_init (SwfdecText * text)
{
  text->glyphs = g_array_new (FALSE, TRUE, sizeof (SwfdecTextGlyph));
  cairo_matrix_init_identity (&text->transform);
}

static void
swfdec_text_dispose (SwfdecText * text)
{
  g_array_free (text->glyphs, TRUE);
}

