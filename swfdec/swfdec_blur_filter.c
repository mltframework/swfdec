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

#include "swfdec_blur_filter.h"

#include <math.h>

#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecBlurFilter, swfdec_blur_filter, SWFDEC_TYPE_FILTER)

static void
swfdec_blur_filter_clone (SwfdecFilter *dfilter, SwfdecFilter *sfilter)
{
  SwfdecBlurFilter *dest = SWFDEC_BLUR_FILTER (dfilter);
  SwfdecBlurFilter *source = SWFDEC_BLUR_FILTER (sfilter);

  dest->x = source->x;
  dest->y = source->y;
  dest->quality = source->quality;
}

static void
swfdec_blur_filter_create_convolution_matrix (SwfdecBlurFilter *blur)
{
  guint x, y, w, h;
  double div, blurx, blury;

  if (blur->matrix)
    return;

  blurx = MAX (blur->x, 1);
  blury = MAX (blur->y, 1);
  w = ceil ((blurx - 1) / 2);
  w = w * 2 + 1;
  h = ceil ((blury - 1) / 2);
  h = h * 2 + 1;

  blur->matrix = swfdec_convolution_matrix_new (w, h);
  div = 1.0 / (blurx * blury);
  for (y = 0; y < h; y++) {
    double val = div;
    if (y == 0 || y == w - 1) {
      val *= (1 - (h - MAX (blur->y, 1)) / 2);
    }
    for (x = 0; x < w; x++) {
      if (x == 0 || x == w - 1) {
	swfdec_convolution_matrix_set (blur->matrix, x, y,
	    val * (1 - (w - MAX (blur->x, 1)) / 2));
      } else {
	swfdec_convolution_matrix_set (blur->matrix, x, y, val);
      }
    }
  }
}

static void
swfdec_blur_filter_get_rectangle (SwfdecFilter *filter, SwfdecRectangle *dest,
    const SwfdecRectangle *source)
{
  SwfdecBlurFilter *blur = SWFDEC_BLUR_FILTER (filter);
  guint x, y;

  swfdec_blur_filter_create_convolution_matrix (blur);
  x = swfdec_convolution_matrix_get_width (blur->matrix) / 2 * blur->quality;
  y = swfdec_convolution_matrix_get_height (blur->matrix) / 2 * blur->quality;

  dest->x = source->x - x;
  dest->y = source->y - y;
  dest->width = source->width + 2 * x;
  dest->height = source->height + 2 * y;
}

static cairo_pattern_t *
swfdec_blur_filter_apply (SwfdecFilter *filter, cairo_pattern_t *pattern,
    const SwfdecRectangle *rect)
{
  SwfdecBlurFilter *blur = SWFDEC_BLUR_FILTER (filter);
  cairo_surface_t *a, *b;
  guint i, j, x, y;
  guint8 *adata, *bdata;
  guint astride, bstride;
  cairo_t *cr;

  if (blur->x <= 1.0 && blur->y <= 1.0)
    return cairo_pattern_reference (pattern);

  swfdec_blur_filter_create_convolution_matrix (blur);
  x = swfdec_convolution_matrix_get_width (blur->matrix) / 2;
  y = swfdec_convolution_matrix_get_height (blur->matrix) / 2;

  /* FIXME: make this work in a single pass (requires smarter matrix construction) */
  a = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
      rect->width + 2 * x * blur->quality,
      rect->height + 2 * y * blur->quality);
  cairo_surface_set_device_offset (a, 
      - (double) rect->x + x * blur->quality, - (double) rect->y + y * blur->quality);
  b = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
      rect->width + 2 * x * blur->quality,
      rect->height + 2 * y * blur->quality);
  cairo_surface_set_device_offset (b, 
      - (double) rect->x + x * blur->quality, - (double) rect->y + y * blur->quality);

  cr = cairo_create (b);
  cairo_set_source (cr, pattern);
  cairo_rectangle (cr, rect->x, rect->y, rect->width, rect->height);
  cairo_fill (cr);
  cairo_destroy (cr);
  cairo_surface_flush (b);

  adata = cairo_image_surface_get_data (a);
  astride = cairo_image_surface_get_stride (a);
  bdata = cairo_image_surface_get_data (b);
  bstride = cairo_image_surface_get_stride (b);
  for (i = 1, j = blur->quality - 1; i <= blur->quality; i++, j--) {
    swfdec_convolution_matrix_apply (blur->matrix,
	rect->width + 2 * x * i, rect->height + 2 * y * i,
	adata + 4 * x * j + astride * y * j, astride,
	bdata + 4 * x * j + bstride * y * j, bstride);
    i++;
    j--;
    if (i > blur->quality) {
      cairo_surface_destroy (b);
      goto out;
    }
    swfdec_convolution_matrix_apply (blur->matrix,
	rect->width + 2 * x * i, rect->height + 2 * y * i,
	bdata + 4 * x * j + bstride * y * j, bstride,
	adata + 4 * x * j + astride * y * j, astride);
  }

  cairo_surface_destroy (a);
  a = b;
out:
  cairo_surface_mark_dirty (a);
  pattern = cairo_pattern_create_for_surface (a);
  cairo_surface_destroy (a);

  return pattern;
}

static void
swfdec_blur_filter_dispose (GObject *object)
{
  SwfdecBlurFilter *blur = SWFDEC_BLUR_FILTER (object);

  if (blur->matrix) {
    swfdec_convolution_matrix_free (blur->matrix);
    blur->matrix = NULL;
  }

  G_OBJECT_CLASS (swfdec_blur_filter_parent_class)->dispose (object);
}

static void
swfdec_blur_filter_class_init (SwfdecBlurFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecFilterClass *filter_class = SWFDEC_FILTER_CLASS (klass);

  object_class->dispose = swfdec_blur_filter_dispose;

  filter_class->clone = swfdec_blur_filter_clone;
  filter_class->get_rectangle = swfdec_blur_filter_get_rectangle;
  filter_class->apply = swfdec_blur_filter_apply;
}

static void
swfdec_blur_filter_init (SwfdecBlurFilter *filter)
{
  filter->x = 4;
  filter->y = 4;
  filter->quality = 1;
}

void
swfdec_blur_filter_invalidate (SwfdecBlurFilter *blur)
{
  g_return_if_fail (SWFDEC_IS_BLUR_FILTER (blur));

  if (blur->matrix) {
    swfdec_convolution_matrix_free (blur->matrix);
    blur->matrix = NULL;
  }
}

