/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include <math.h>

#include "swfdec_stroke.h"
#include "swfdec_bits.h"
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_image.h"

#define MAX_ALIGN 10

G_DEFINE_TYPE (SwfdecStroke, swfdec_stroke, G_TYPE_OBJECT);

static void
swfdec_stroke_append_path_snapped (cairo_t *cr, const cairo_path_t *path)
{
  cairo_path_data_t *data;
  double x, y;
  int i;

  data = path->data;
  for (i = 0; i < path->num_data; i++) {
    switch (data[i].header.type) {
      case CAIRO_PATH_MOVE_TO:
	i++;
	x = data[i].point.x;
	y = data[i].point.y;
	cairo_user_to_device (cr, &x, &y);
	x = rint (x - 0.5) + 0.5;
	y = rint (y - 0.5) + 0.5;
	cairo_device_to_user (cr, &x, &y);
	/* FIXME: currently we need to clamp this due to extents */
	x = CLAMP (x, data[i].point.x - MAX_ALIGN, data[i].point.x + MAX_ALIGN);
	y = CLAMP (y, data[i].point.y - MAX_ALIGN, data[i].point.y + MAX_ALIGN);
	cairo_move_to (cr, x, y);
	break;
      case CAIRO_PATH_LINE_TO:
	i++;
	x = data[i].point.x;
	y = data[i].point.y;
	cairo_user_to_device (cr, &x, &y);
	x = rint (x - 0.5) + 0.5;
	y = rint (y - 0.5) + 0.5;
	cairo_device_to_user (cr, &x, &y);
	/* FIXME: currently we need to clamp this due to extents */
	x = CLAMP (x, data[i].point.x - MAX_ALIGN, data[i].point.x + MAX_ALIGN);
	y = CLAMP (y, data[i].point.y - MAX_ALIGN, data[i].point.y + MAX_ALIGN);
	cairo_line_to (cr, x, y);
	break;
      case CAIRO_PATH_CURVE_TO:
	x = data[i+3].point.x;
	y = data[i+3].point.y;
	cairo_user_to_device (cr, &x, &y);
	x = rint (x - 0.5) + 0.5;
	y = rint (y - 0.5) + 0.5;
	cairo_device_to_user (cr, &x, &y);
	/* FIXME: currently we need to clamp this due to extents */
	x = CLAMP (x, data[i+3].point.x - MAX_ALIGN, data[i+3].point.x + MAX_ALIGN);
	y = CLAMP (y, data[i+3].point.y - MAX_ALIGN, data[i+3].point.y + MAX_ALIGN);
	cairo_curve_to (cr, data[i+1].point.x, data[i+1].point.y, 
	    data[i+2].point.x, data[i+2].point.y, x, y);
	i += 3;
	break;
      case CAIRO_PATH_CLOSE_PATH:
	/* doesn't exist in our code */
      default:
	g_assert_not_reached ();
    }
  }
}

void
swfdec_stroke_paint (SwfdecStroke *stroke, cairo_t *cr, const cairo_path_t *path,
    const SwfdecColorTransform *trans, guint ratio)
{
  SwfdecColor color;
  double width;

  swfdec_stroke_append_path_snapped (cr, path);
  color = swfdec_color_apply_morph (stroke->start_color, stroke->end_color, ratio);
  color = swfdec_color_apply_transform (color, trans);
  swfdec_color_set_source (cr, color);
  if (ratio == 0) {
    width = stroke->start_width;
  } else if (ratio == 65535) {
    width = stroke->end_width;
  } else {
    width = (stroke->start_width * (65535 - ratio) + stroke->end_width * ratio) / 65535;
  }
  if (width < SWFDEC_TWIPS_SCALE_FACTOR)
    width = SWFDEC_TWIPS_SCALE_FACTOR;
  cairo_set_line_width (cr, width);
  cairo_stroke (cr);
}

static void
swfdec_stroke_class_init (SwfdecStrokeClass *klass)
{
}

static void
swfdec_stroke_init (SwfdecStroke *stroke)
{
}

/*** EXPORTED API ***/

SwfdecStroke *
swfdec_stroke_parse (SwfdecSwfDecoder *dec, gboolean rgba)
{
  SwfdecBits *bits = &dec->b;
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  stroke->start_width = swfdec_bits_get_u16 (bits);
  stroke->end_width = stroke->start_width;
  if (rgba) {
    stroke->start_color = swfdec_bits_get_rgba (bits);
  } else {
    stroke->start_color = swfdec_bits_get_color (bits);
  }
  stroke->end_color = stroke->start_color;
  SWFDEC_LOG ("new stroke stroke: width %u color %08x", stroke->start_width, stroke->start_color);

  return stroke;
}

SwfdecStroke *
swfdec_stroke_parse_morph (SwfdecSwfDecoder *dec)
{
  SwfdecBits *bits = &dec->b;
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  stroke->start_width = swfdec_bits_get_u16 (bits);
  stroke->end_width = swfdec_bits_get_u16 (bits);
  stroke->start_color = swfdec_bits_get_rgba (bits);
  stroke->end_color = swfdec_bits_get_rgba (bits);
  SWFDEC_LOG ("new stroke stroke: width %u => %u color %08X => %08X", 
      stroke->start_width, stroke->end_width,
      stroke->start_color, stroke->end_color);

  return stroke;
}

SwfdecStroke *
swfdec_stroke_new (guint width, SwfdecColor color)
{
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  stroke->start_width = width;
  stroke->end_width = width;
  stroke->start_color = color;
  stroke->end_color = color;

  return stroke;
}

static SwfdecStroke *
swfdec_stroke_do_parse_extended (SwfdecBits *bits, gboolean morph,
    SwfdecPattern *fill_styles, guint n_fill_styles)
{
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  SWFDEC_ERROR ("FIXME: implement");
  return stroke;
}

SwfdecStroke *
swfdec_stroke_parse_extended (SwfdecSwfDecoder *dec, SwfdecPattern *fill_styles,
    guint n_fill_styles)
{
  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (dec), NULL);
  g_return_val_if_fail (n_fill_styles == 0 || fill_styles != NULL, NULL);

  return swfdec_stroke_do_parse_extended (&dec->b, FALSE, fill_styles, n_fill_styles);
}

SwfdecStroke *
swfdec_stroke_parse_morph_extended (SwfdecSwfDecoder *dec, SwfdecPattern *fill_styles,
    guint n_fill_styles)
{
  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (dec), NULL);
  g_return_val_if_fail (n_fill_styles == 0 || fill_styles != NULL, NULL);

  return swfdec_stroke_do_parse_extended (&dec->b, TRUE, fill_styles, n_fill_styles);
}
