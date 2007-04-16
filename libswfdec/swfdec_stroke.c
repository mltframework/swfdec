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

  g_return_if_fail (SWFDEC_IS_STROKE (stroke));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (path != NULL);
  g_return_if_fail (trans != NULL);
  g_return_if_fail (ratio < 65536);

  cairo_set_line_cap (cr, stroke->start_cap);
  cairo_set_line_join (cr, stroke->join);
  if (stroke->join == CAIRO_LINE_JOIN_MITER)
    cairo_set_miter_limit (cr, stroke->miter_limit);

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
  stroke->start_cap = CAIRO_LINE_CAP_ROUND;
  stroke->end_cap = CAIRO_LINE_CAP_ROUND;
  stroke->join = CAIRO_LINE_JOIN_ROUND;
}

/*** EXPORTED API ***/

SwfdecStroke *
swfdec_stroke_parse (SwfdecSwfDecoder *dec)
{
  SwfdecBits *bits = &dec->b;
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  stroke->start_width = swfdec_bits_get_u16 (bits);
  stroke->end_width = stroke->start_width;
  stroke->start_color = swfdec_bits_get_color (bits);
  stroke->end_color = stroke->start_color;
  SWFDEC_LOG ("new stroke: width %u color %08x", stroke->start_width, stroke->start_color);

  return stroke;
}

SwfdecStroke *
swfdec_stroke_parse_rgba (SwfdecSwfDecoder *dec)
{
  SwfdecBits *bits = &dec->b;
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  stroke->start_width = swfdec_bits_get_u16 (bits);
  stroke->end_width = stroke->start_width;
  stroke->start_color = swfdec_bits_get_rgba (bits);
  stroke->end_color = stroke->start_color;
  SWFDEC_LOG ("new stroke: width %u color %08x", stroke->start_width, stroke->start_color);

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
  SWFDEC_LOG ("new stroke: width %u => %u color %08X => %08X", 
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

static cairo_line_cap_t
swfdec_line_cap_get (guint cap)
{
  switch (cap) {
    case 0:
      return CAIRO_LINE_CAP_ROUND;
    case 1:
      return CAIRO_LINE_CAP_BUTT;
    case 2:
      return CAIRO_LINE_CAP_SQUARE;
    default:
      SWFDEC_ERROR ("invalid line cap value %u", cap);
      return CAIRO_LINE_CAP_ROUND;
  }
}
static cairo_line_join_t
swfdec_line_join_get (guint join)
{
  switch (join) {
    case 0:
      return CAIRO_LINE_JOIN_ROUND;
    case 1:
      return CAIRO_LINE_JOIN_BEVEL;
    case 2:
      return CAIRO_LINE_JOIN_MITER;
    default:
      SWFDEC_ERROR ("invalid line join value %u", join);
      return CAIRO_LINE_JOIN_ROUND;
  }
}

static SwfdecStroke *
swfdec_stroke_do_parse_extended (SwfdecSwfDecoder *dec, gboolean morph)
{
  SwfdecBits *bits = &dec->b;
  guint tmp;
  gboolean has_pattern;
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  stroke->start_width = swfdec_bits_get_u16 (bits);
  if (morph) {
    stroke->end_width = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  width: %u => %u", stroke->start_width, stroke->end_width);
  } else {
    stroke->end_width = stroke->start_width;
    SWFDEC_LOG ("  width: %u", stroke->start_width);
  }
  tmp = swfdec_bits_getbits (bits, 2);
  SWFDEC_LOG ("  start cap: %u", tmp);
  stroke->start_cap = swfdec_line_cap_get (tmp);
  tmp = swfdec_bits_getbits (bits, 2);
  SWFDEC_LOG ("  line join: %u", tmp);
  stroke->join = swfdec_line_join_get (tmp);
  has_pattern = swfdec_bits_getbit (bits);
  SWFDEC_LOG ("  has pattern: %d", has_pattern);
  stroke->no_hscale = swfdec_bits_getbit (bits);
  SWFDEC_LOG ("  no hscale: %d", stroke->no_hscale);
  stroke->no_vscale = swfdec_bits_getbit (bits);
  SWFDEC_LOG ("  no vscale: %d", stroke->no_vscale);
  stroke->align_pixel = swfdec_bits_getbit (bits);
  SWFDEC_LOG ("  align pixels: %d", stroke->align_pixel);
  tmp = swfdec_bits_getbits (bits, 5);
  stroke->no_close = swfdec_bits_getbit (bits);
  SWFDEC_LOG ("  no close: %d", stroke->no_close);
  tmp = swfdec_bits_getbits (bits, 2);
  SWFDEC_LOG ("  end cap: %u", tmp);
  stroke->end_cap = swfdec_line_cap_get (tmp);
  if (stroke->end_cap != stroke->start_cap) {
    SWFDEC_WARNING ("FIXME: different caps on start and end of line are unsupported");
  }
  if (stroke->join == CAIRO_LINE_JOIN_MITER) {
    stroke->miter_limit = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  miter limit: %u", stroke->miter_limit);
  }
  if (has_pattern) {
    if (morph) {
      stroke->pattern = swfdec_pattern_parse_morph (dec);
    } else {
      stroke->pattern = swfdec_pattern_parse_rgba (dec);
    }
  } else {
    stroke->start_color = swfdec_bits_get_rgba (bits);
    if (morph) {
      stroke->end_color = swfdec_bits_get_rgba (bits);
      SWFDEC_LOG ("  color: #%08X", stroke->start_color);
    } else {
      stroke->end_color = stroke->start_color;
      SWFDEC_LOG ("  color: #%08X", stroke->start_color);
    }
  }

  return stroke;
}

SwfdecStroke *
swfdec_stroke_parse_extended (SwfdecSwfDecoder *dec)
{
  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (dec), NULL);

  return swfdec_stroke_do_parse_extended (dec, FALSE);
}

SwfdecStroke *
swfdec_stroke_parse_morph_extended (SwfdecSwfDecoder *dec)
{
  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (dec), NULL);

  return swfdec_stroke_do_parse_extended (dec, TRUE);
}
