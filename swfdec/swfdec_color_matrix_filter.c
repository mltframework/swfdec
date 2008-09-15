/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_color_matrix_filter.h"

#include <string.h>

#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecColorMatrixFilter, swfdec_color_matrix_filter, SWFDEC_TYPE_FILTER)

static void
swfdec_color_matrix_filter_clone (SwfdecFilter *dfilter, SwfdecFilter *sfilter)
{
  SwfdecColorMatrixFilter *dest = SWFDEC_COLOR_MATRIX_FILTER (dfilter);
  SwfdecColorMatrixFilter *source = SWFDEC_COLOR_MATRIX_FILTER (sfilter);

  memcpy (dest->matrix, source->matrix, sizeof (double) * 20);
}

static void
swfdec_color_matrix_filter_get_rectangle (SwfdecFilter *filter, SwfdecRectangle *dest,
    const SwfdecRectangle *source)
{
  if (dest != source)
    *dest = *source;
}

static cairo_pattern_t *
swfdec_color_matrix_filter_apply (SwfdecFilter *filter, cairo_pattern_t *pattern,
    const SwfdecRectangle *rect)
{
  SwfdecColorMatrixFilter *cm = SWFDEC_COLOR_MATRIX_FILTER (filter);
  cairo_surface_t *surface;
  int x, y;
  guint8 *data;
  guint stride;
  cairo_t *cr;
  double a, r, g, b, anew, tmp;

  /* FIXME: make this work in a single pass (requires smarter matrix construction) */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, rect->width, rect->height);
  cairo_surface_set_device_offset (surface, -rect->x, -rect->y);

  cr = cairo_create (surface);
  cairo_set_source (cr, pattern);
  cairo_rectangle (cr, rect->x, rect->y, rect->width, rect->height);
  cairo_fill (cr);
  cairo_destroy (cr);

  data = cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);
  for (y = 0; y < rect->height; y++) {
    for (x = 0; x < rect->height; x++) {
      a = data[x * 4 + SWFDEC_COLOR_INDEX_ALPHA];
      r = data[x * 4 + SWFDEC_COLOR_INDEX_RED];
      g = data[x * 4 + SWFDEC_COLOR_INDEX_GREEN];
      b = data[x * 4 + SWFDEC_COLOR_INDEX_BLUE];
      anew = r * cm->matrix[15] + g * cm->matrix[16] + b * cm->matrix[17] +
	a * cm->matrix[18] + cm->matrix[19];
      anew = CLAMP (anew, 0, 255);
      data[x * 4 + SWFDEC_COLOR_INDEX_ALPHA] = anew;
      tmp = r * cm->matrix[0] + g * cm->matrix[1] + b * cm->matrix[2] +
	a * cm->matrix[3] + cm->matrix[4];
      tmp = CLAMP (tmp, 0, anew);
      data[x * 4 + SWFDEC_COLOR_INDEX_RED] = tmp;
      tmp = r * cm->matrix[5] + g * cm->matrix[6] + b * cm->matrix[7] +
	a * cm->matrix[8] + cm->matrix[9];
      tmp = CLAMP (tmp, 0, anew);
      data[x * 4 + SWFDEC_COLOR_INDEX_GREEN] = tmp;
      tmp = r * cm->matrix[10] + g * cm->matrix[11] + b * cm->matrix[12] +
	a * cm->matrix[13] + cm->matrix[14];
      tmp = CLAMP (tmp, 0, anew);
      data[x * 4 + SWFDEC_COLOR_INDEX_BLUE] = tmp;
    }
    data += stride;
  }

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_surface_destroy (surface);

  return pattern;
}

static void
swfdec_color_matrix_filter_class_init (SwfdecColorMatrixFilterClass *klass)
{
  SwfdecFilterClass *filter_class = SWFDEC_FILTER_CLASS (klass);

  filter_class->clone = swfdec_color_matrix_filter_clone;
  filter_class->get_rectangle = swfdec_color_matrix_filter_get_rectangle;
  filter_class->apply = swfdec_color_matrix_filter_apply;
}

static void
swfdec_color_matrix_filter_init (SwfdecColorMatrixFilter *filter)
{
  filter->matrix[0] = 1.0;
  filter->matrix[6] = 1.0;
  filter->matrix[12] = 1.0;
  filter->matrix[18] = 1.0;
}

