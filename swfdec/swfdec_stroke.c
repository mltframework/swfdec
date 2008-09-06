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
#include "swfdec_path.h"
#include "swfdec_renderer_internal.h"

#define MAX_ALIGN 10

G_DEFINE_TYPE (SwfdecStroke, swfdec_stroke, SWFDEC_TYPE_DRAW);

/* FIXME: This is wrong. Snapping only happens for vertical or horizontal lines.
 * And the snapping shouldn't happen to the cr's device units, but to the 
 * Stage coordinate system.
 * This would of course require this function to know that matrix... */
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

static void
swfdec_stroke_paint (SwfdecDraw *draw, cairo_t *cr, const SwfdecColorTransform *trans)
{
  SwfdecStroke *stroke = SWFDEC_STROKE (draw);
  SwfdecColor color;

  cairo_set_line_cap (cr, stroke->start_cap);
  cairo_set_line_join (cr, stroke->join);
  if (stroke->join == CAIRO_LINE_JOIN_MITER)
    cairo_set_miter_limit (cr, stroke->miter_limit);

  if (draw->snap)
    swfdec_stroke_append_path_snapped (cr, &draw->path);
  else
    cairo_append_path (cr, &draw->path);

  if (stroke->pattern) {
    cairo_pattern_t *pattern = swfdec_pattern_get_pattern (stroke->pattern, 
	swfdec_renderer_get (cr), trans);
    cairo_set_source (cr, pattern);
    cairo_pattern_destroy (pattern);
  } else {
    color = swfdec_color_apply_transform (stroke->start_color, trans);
    swfdec_color_set_source (cr, color);
  }
  cairo_set_line_width (cr, MAX (stroke->start_width, SWFDEC_TWIPS_SCALE_FACTOR));
  cairo_stroke (cr);
}

static void
swfdec_stroke_morph (SwfdecDraw *dest, SwfdecDraw *source, guint ratio)
{
  SwfdecStroke *dstroke = SWFDEC_STROKE (dest);
  SwfdecStroke *sstroke = SWFDEC_STROKE (source);

  dstroke->start_color = swfdec_color_apply_morph (sstroke->start_color,
      sstroke->end_color, ratio);
  dstroke->end_color = dstroke->start_color;
  dstroke->start_width = (sstroke->start_width * ratio + 
      sstroke->end_width * (65535 - ratio)) / 65535;
  dstroke->end_width = dstroke->start_width;
  if (sstroke->pattern) {
    dstroke->pattern = SWFDEC_PATTERN (swfdec_draw_morph (
	  SWFDEC_DRAW (sstroke->pattern), ratio));
  }
  dstroke->start_cap = sstroke->start_cap;
  dstroke->end_cap = sstroke->end_cap;
  dstroke->join = sstroke->join;
  dstroke->miter_limit = sstroke->miter_limit;
  dstroke->no_vscale = sstroke->no_vscale;
  dstroke->no_hscale = sstroke->no_hscale;
  dstroke->no_close = sstroke->no_close;

  SWFDEC_DRAW_CLASS (swfdec_stroke_parent_class)->morph (dest, source, ratio);
}

static void
swfdec_stroke_compute_extents (SwfdecDraw *draw)
{
  guint width = SWFDEC_STROKE (draw)->start_width;

  if (SWFDEC_STROKE (draw)->join != CAIRO_LINE_JOIN_ROUND) {
    SWFDEC_FIXME ("work out extents computation for non-round line joins");
  }
  swfdec_path_get_extents (&draw->path, &draw->extents);
  draw->extents.x0 -= width;
  draw->extents.x1 += width;
  draw->extents.y0 -= width;
  draw->extents.y1 += width;
}

static gboolean
swfdec_stroke_contains (SwfdecDraw *draw, cairo_t *cr, double x, double y)
{
  SwfdecStroke *stroke = SWFDEC_STROKE (draw);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_cap (cr, stroke->start_cap);
  cairo_set_line_join (cr, stroke->join);
  if (stroke->join == CAIRO_LINE_JOIN_MITER)
    cairo_set_miter_limit (cr, stroke->miter_limit);
  cairo_set_line_width (cr, MAX (stroke->start_width, SWFDEC_TWIPS_SCALE_FACTOR));
  /* FIXME: do snapping here? */
  cairo_append_path (cr, &draw->path);
  return cairo_in_stroke (cr, x, y);
}

static void
swfdec_stroke_class_init (SwfdecStrokeClass *klass)
{
  SwfdecDrawClass *draw_class = SWFDEC_DRAW_CLASS (klass);

  draw_class->morph = swfdec_stroke_morph;
  draw_class->paint = swfdec_stroke_paint;
  draw_class->compute_extents = swfdec_stroke_compute_extents;
  draw_class->contains = swfdec_stroke_contains;
}

static void
swfdec_stroke_init (SwfdecStroke *stroke)
{
  SwfdecDraw *draw = SWFDEC_DRAW (stroke);

  draw->snap = TRUE;

  stroke->start_cap = CAIRO_LINE_CAP_ROUND;
  stroke->end_cap = CAIRO_LINE_CAP_ROUND;
  stroke->join = CAIRO_LINE_JOIN_ROUND;
}

/*** EXPORTED API ***/

SwfdecDraw *
swfdec_stroke_parse (SwfdecBits *bits, SwfdecSwfDecoder *dec)
{
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  stroke->start_width = swfdec_bits_get_u16 (bits);
  stroke->end_width = stroke->start_width;
  stroke->start_color = swfdec_bits_get_color (bits);
  stroke->end_color = stroke->start_color;
  SWFDEC_LOG ("new stroke: width %u color %08x", stroke->start_width, stroke->start_color);

  return SWFDEC_DRAW (stroke);
}

SwfdecDraw *
swfdec_stroke_parse_rgba (SwfdecBits *bits, SwfdecSwfDecoder *dec)
{
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  stroke->start_width = swfdec_bits_get_u16 (bits);
  stroke->end_width = stroke->start_width;
  stroke->start_color = swfdec_bits_get_rgba (bits);
  stroke->end_color = stroke->start_color;
  SWFDEC_LOG ("new stroke: width %u color %08x", stroke->start_width, stroke->start_color);

  return SWFDEC_DRAW (stroke);
}

SwfdecDraw *
swfdec_stroke_parse_morph (SwfdecBits *bits, SwfdecSwfDecoder *dec)
{
  SwfdecStroke *stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  stroke->start_width = swfdec_bits_get_u16 (bits);
  stroke->end_width = swfdec_bits_get_u16 (bits);
  stroke->start_color = swfdec_bits_get_rgba (bits);
  stroke->end_color = swfdec_bits_get_rgba (bits);
  SWFDEC_LOG ("new stroke: width %u => %u color %08X => %08X", 
      stroke->start_width, stroke->end_width,
      stroke->start_color, stroke->end_color);

  return SWFDEC_DRAW (stroke);
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

static SwfdecDraw *
swfdec_stroke_do_parse_extended (SwfdecBits *bits, SwfdecSwfDecoder *dec, gboolean morph)
{
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
  SWFDEC_DRAW (stroke)->snap = swfdec_bits_getbit (bits);
  SWFDEC_LOG ("  align pixels: %d", SWFDEC_DRAW (stroke)->snap);
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
      stroke->pattern = SWFDEC_PATTERN (swfdec_pattern_parse_morph (bits, dec));
    } else {
      stroke->pattern = SWFDEC_PATTERN (swfdec_pattern_parse_rgba (bits, dec));
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

  return SWFDEC_DRAW (stroke);
}

SwfdecDraw *
swfdec_stroke_parse_extended (SwfdecBits *bits, SwfdecSwfDecoder *dec)
{
  g_return_val_if_fail (bits != NULL, NULL);
  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (dec), NULL);

  return swfdec_stroke_do_parse_extended (bits, dec, FALSE);
}

SwfdecDraw *
swfdec_stroke_parse_morph_extended (SwfdecBits *bits, SwfdecSwfDecoder *dec)
{
  g_return_val_if_fail (bits != NULL, NULL);
  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (dec), NULL);

  return swfdec_stroke_do_parse_extended (bits, dec, TRUE);
}
