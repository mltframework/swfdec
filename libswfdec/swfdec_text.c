
#include <swfdec_text.h>
#include "swfdec_internal.h"
#include <swfdec_font.h>

static gboolean
swfdec_text_mouse_in (SwfdecObject *object,
      double x, double y, int button)
{
  unsigned int i;
  SwfdecObject *fontobj;
  SwfdecText *text = SWFDEC_TEXT (object);

  swfdec_matrix_transform_point_inverse (&text->transform, &x, &y);
  for (i = 0; i < text->glyphs->len; i++) {
    SwfdecTextGlyph *glyph;
    SwfdecShape *shape;
    double tmpx, tmpy;

    glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);
    fontobj = swfdec_object_get (object->decoder, glyph->font);
    if (fontobj == NULL)
      continue;

    shape = swfdec_font_get_glyph (SWFDEC_FONT (fontobj), glyph->glyph);
    tmpx = x - glyph->x;
    tmpy = y - glyph->y;
    tmpx /= SWF_TEXT_SCALE_FACTOR * glyph->height;
    tmpy /= SWF_TEXT_SCALE_FACTOR * glyph->height;
    if (swfdec_object_mouse_in (SWFDEC_OBJECT (shape), tmpx, tmpy, button))
      return TRUE;
  }
  return FALSE;
}

static void
swfdec_text_render (SwfdecObject *obj, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  unsigned int i, color;
  SwfdecText *text = SWFDEC_TEXT (obj);
  SwfdecObject *fontobj;
  SwfdecColorTransform force_color;
  SwfdecRect rect, inval_moved;

  cairo_transform (cr, &text->transform);
  /* scale by bounds */
#if 0
  cairo_translate (cr, obj->extents.x0, obj->extents.y0);
  inval_moved.x0 = inval->x0 - obj->extents.x0;
  inval_moved.y0 = inval->y0 - obj->extents.y0;
  inval_moved.x1 = inval->x1 - obj->extents.x0;
  inval_moved.y1 = inval->y1 - obj->extents.y0;
#endif
  swfdec_rect_transform_inverse (&inval_moved, inval, &text->transform);
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

    cairo_matrix_init_translate (&pos,
	glyph->x, glyph->y);
    cairo_matrix_scale (&pos, 
	glyph->height * SWF_TEXT_SCALE_FACTOR, 
	glyph->height * SWF_TEXT_SCALE_FACTOR);
    cairo_save (cr);
    cairo_transform (cr, &pos);
    swfdec_rect_transform_inverse (&rect, &inval_moved, &pos);
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
  object_class->mouse_in = swfdec_text_mouse_in;
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

