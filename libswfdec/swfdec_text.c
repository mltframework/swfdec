
#include <swfdec_text.h>
#include "swfdec_internal.h"
#include <swfdec_font.h>

static SwfdecMouseResult 
swfdec_text_handle_mouse (SwfdecObject *object,
      double x, double y, int button)
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
    fontobj = swfdec_object_get (object->decoder, glyph->font);
    if (fontobj == NULL)
      continue;

    shape = swfdec_font_get_glyph (SWFDEC_FONT (fontobj), glyph->glyph);
    ret = swfdec_object_handle_mouse (SWFDEC_OBJECT (shape),
	x - glyph->x, y - glyph->y * SWF_SCALE_FACTOR, button, TRUE);
    if (ret != SWFDEC_MOUSE_MISSED)
      return ret;
  }
  return SWFDEC_MOUSE_MISSED;
}

static void
swfdec_text_render (SwfdecObject *obj, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  unsigned int i, color;
  SwfdecText *text = SWFDEC_TEXT (obj);
  SwfdecObject *fontobj;
  SwfdecColorTransform force_color;
  SwfdecRect rect;

  //cairo_transform (cr, &text->transform);
  g_print ("rendering text %d\n", obj->id);
  /* scale by bounds */
  for (i = 0; i < text->glyphs->len; i++) {
    SwfdecTextGlyph *glyph;
    SwfdecShape *shape;
    cairo_matrix_t pos;

    glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);

    fontobj = swfdec_object_get (obj->decoder, glyph->font);
    if (fontobj == NULL)
      continue;

    shape = swfdec_font_get_glyph (SWFDEC_FONT (fontobj), glyph->glyph);
    if (shape == NULL) {
      SWFDEC_ERROR ("failed getting glyph %d\n", glyph->glyph);
      continue;
    }

    g_print ("%d %d %d\n", glyph->x, glyph->y, glyph->height);
    cairo_matrix_init_scale (&pos, glyph->height * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR, glyph->height * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR);
    cairo_matrix_translate (&pos,
	glyph->x * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR, glyph->y * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR);
    //pos.xx = glyph->height * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR;
    //pos.yy = glyph->height * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR;
    cairo_save (cr);
    {
      double x = glyph->x, y = glyph->y;
      cairo_user_to_device_distance (cr, &x, &y);
      g_print ("--> %g %g\n", x, y);
    }
    cairo_scale (cr, glyph->height * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR, glyph->height * SWF_TEXT_SCALE_FACTOR / SWF_SCALE_FACTOR);
    cairo_translate (cr, glyph->x, glyph->y);
    //cairo_transform (cr, &pos);
    swfdec_rect_transform_inverse (&rect, inval, &pos);
    color = swfdec_color_apply_transform (glyph->color, trans);
    swfdec_color_transform_init_color (&force_color, color);
    swfdec_object_render (SWFDEC_OBJECT (shape), cr, 
	&force_color, &rect);
    cairo_restore (cr);
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

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (text));
}

