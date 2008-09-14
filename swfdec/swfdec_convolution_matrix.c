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

#include "swfdec_convolution_matrix.h"

#include "swfdec_color.h"
#include "swfdec_debug.h"

struct _SwfdecConvolutionMatrix {
  guint		width;
  guint		height;
  double      	matrix[1];
};

SwfdecConvolutionMatrix *
swfdec_convolution_matrix_new (guint width, guint height)
{
  SwfdecConvolutionMatrix *matrix;

  g_return_val_if_fail (width % 2 != 0, NULL);
  g_return_val_if_fail (height % 2 != 0, NULL);

  matrix = g_slice_alloc0 (sizeof (SwfdecConvolutionMatrix) + 
      sizeof (guint8) * (width * height * 256 - 1));
  matrix->width = width;
  matrix->height = height;
  return matrix;
}

void
swfdec_convolution_matrix_free (SwfdecConvolutionMatrix *matrix)
{
  g_return_if_fail (matrix != NULL);

  g_slice_free1 (sizeof (SwfdecConvolutionMatrix) + 
      sizeof (guint8) * (matrix->width * matrix->height * 256 - 1), matrix);
}

void
swfdec_convolution_matrix_set (SwfdecConvolutionMatrix *matrix,
    guint x, guint y, double value)
{
  g_return_if_fail (matrix != NULL);
  g_return_if_fail (x < matrix->width);
  g_return_if_fail (y < matrix->height);

  matrix->matrix[matrix->width * y + x] = value;
}

double
swfdec_convolution_matrix_get (SwfdecConvolutionMatrix *matrix, guint x, guint y)
{
  g_return_val_if_fail (matrix != NULL, 0);
  g_return_val_if_fail (x < matrix->width, 0);
  g_return_val_if_fail (y < matrix->height, 0);

  return matrix->matrix[matrix->width * y + x];
}

guint
swfdec_convolution_matrix_get_width (SwfdecConvolutionMatrix *matrix)
{
  g_return_val_if_fail (matrix != NULL, 1);

  return matrix->width;
}

guint
swfdec_convolution_matrix_get_height (SwfdecConvolutionMatrix *matrix)
{
  g_return_val_if_fail (matrix != NULL, 1);

  return matrix->height;
}

void
swfdec_convolution_matrix_apply (SwfdecConvolutionMatrix *matrix, guint width, guint height, 
    guint8 *tdata, guint tstride, const guint8 *sdata, guint sstride)
{
  double r, g, b, a;
  guint x, y;
  int offx, offy, offyend, offxend;

  g_return_if_fail (matrix != NULL);
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);
  g_return_if_fail (tdata != NULL);
  g_return_if_fail (tstride >= width * 4);
  g_return_if_fail (sdata != NULL);
  g_return_if_fail (sstride >= width * 4);

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      double *next = matrix->matrix;
      a = r = g = b = 0;
      offy = y - matrix->height / 2;
      offyend = offy + matrix->height;
      for (; offy < offyend; offy++) {
	if (offy < 0 || (guint) offy >= height) {
	  next += matrix->width;
	  continue;
	}
	offx = x - matrix->width / 2;
	offxend = offx + matrix->width;
	for (; offx < offxend; offx++) {
	  const guint8 *cur;
	  if (offx < 0 || (guint) offx >= width) {
	    next++;
	    continue;
	  }
	  cur = &sdata[4 * offx + sstride * offy];
	  g_assert (cur < sdata + sstride * height);
	  a += *next * cur[SWFDEC_COLOR_INDEX_ALPHA];
	  r += *next * cur[SWFDEC_COLOR_INDEX_RED];
	  g += *next * cur[SWFDEC_COLOR_INDEX_GREEN];
	  b += *next * cur[SWFDEC_COLOR_INDEX_BLUE];
	  next++;
	}
      }
      a = CLAMP (a, 0, 255);
      r = CLAMP (r, 0, a);
      g = CLAMP (g, 0, a);
      b = CLAMP (b, 0, a);
      tdata[4 * x + SWFDEC_COLOR_INDEX_ALPHA] = a;
      tdata[4 * x + SWFDEC_COLOR_INDEX_RED] = r;
      tdata[4 * x + SWFDEC_COLOR_INDEX_GREEN] = g;
      tdata[4 * x + SWFDEC_COLOR_INDEX_BLUE] = b;
    }
    tdata += tstride;
  }
}

