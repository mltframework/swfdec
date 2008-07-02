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

#include "swfdec_text.h"
#include "swfdec_debug.h"
#include "swfdec_draw.h"
#include "swfdec_font.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecText, swfdec_text, SWFDEC_TYPE_GRAPHIC)

static gboolean
swfdec_text_mouse_in (SwfdecGraphic *graphic, double x, double y)
{
  guint i;
  SwfdecText *text = SWFDEC_TEXT (graphic);

  cairo_matrix_transform_point (&text->transform_inverse, &x, &y);
  for (i = 0; i < text->glyphs->len; i++) {
    SwfdecTextGlyph *glyph;
    SwfdecDraw *draw;
    double tmpx, tmpy;

    glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);
    draw = swfdec_font_get_glyph (glyph->font, glyph->glyph);
    if (draw == NULL)
      continue;
    tmpx = x - glyph->x;
    tmpy = y - glyph->y;
    tmpx = tmpx * glyph->font->scale_factor / glyph->height;
    tmpy = tmpy * glyph->font->scale_factor / glyph->height;
    if (swfdec_draw_contains (draw, tmpx, tmpy))
      return TRUE;
  }
  return FALSE;
}

static void
swfdec_text_render (SwfdecGraphic *graphic, cairo_t *cr, 
    const SwfdecColorTransform *trans)
{
  guint i;
  SwfdecColor color;
  SwfdecText *text = SWFDEC_TEXT (graphic);
  SwfdecColorTransform force_color;

  cairo_transform (cr, &text->transform);
  /* scale by bounds */
  for (i = 0; i < text->glyphs->len; i++) {
    SwfdecTextGlyph *glyph;
    SwfdecDraw *draw;
    cairo_matrix_t pos;

    glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);

    draw = swfdec_font_get_glyph (glyph->font, glyph->glyph);
    if (draw == NULL) {
      SWFDEC_INFO ("failed getting glyph %d, maybe an empty glyph?", glyph->glyph);
      continue;
    }

    cairo_matrix_init_translate (&pos,
	glyph->x, glyph->y);
    cairo_matrix_scale (&pos, 
	(double) glyph->height / glyph->font->scale_factor,
	(double) glyph->height / glyph->font->scale_factor);
    cairo_save (cr);
    cairo_transform (cr, &pos);
    if (!cairo_matrix_invert (&pos)) {
      color = swfdec_color_apply_transform (glyph->color, trans);
      swfdec_color_transform_init_color (&force_color, color);
      swfdec_draw_paint (draw, cr, &force_color);
    } else {
      SWFDEC_ERROR ("non-invertible matrix!");
    }
    cairo_restore (cr);
  }
}

static void
swfdec_text_dispose (GObject *object)
{
  SwfdecText * text = SWFDEC_TEXT (object);

  g_array_free (text->glyphs, TRUE);

  G_OBJECT_CLASS (swfdec_text_parent_class)->dispose (object);
}

static void
swfdec_text_class_init (SwfdecTextClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);

  object_class->dispose = swfdec_text_dispose;

  graphic_class->render = swfdec_text_render;
  graphic_class->mouse_in = swfdec_text_mouse_in;
}

static void
swfdec_text_init (SwfdecText * text)
{
  text->glyphs = g_array_new (FALSE, TRUE, sizeof (SwfdecTextGlyph));
  cairo_matrix_init_identity (&text->transform);
}

