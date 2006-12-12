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

#include <math.h>
#include "swfdec_color.h"
#include "swfdec_debug.h"

SwfdecColor 
swfdec_color_apply_morph (SwfdecColor start, SwfdecColor end, unsigned int ratio)
{
  unsigned int r, g, b, a;
  unsigned int start_ratio, end_ratio;

  g_assert (ratio < 65536);
  if (ratio == 0)
    return start;
  if (ratio == 65535)
    return end;
  start_ratio = 65535 - ratio;
  end_ratio = ratio;
  r = (SWF_COLOR_R (start) * start_ratio + SWF_COLOR_R (end) * end_ratio) / 65535;
  g = (SWF_COLOR_G (start) * start_ratio + SWF_COLOR_G (end) * end_ratio) / 65535;
  b = (SWF_COLOR_B (start) * start_ratio + SWF_COLOR_B (end) * end_ratio) / 65535;
  a = (SWF_COLOR_A (start) * start_ratio + SWF_COLOR_A (end) * end_ratio) / 65535;

  return SWF_COLOR_COMBINE (r, g, b, a);
}

void 
swfdec_color_set_source (cairo_t *cr, SwfdecColor color)
{
  cairo_set_source_rgba (cr, 
      SWF_COLOR_R (color) / 255.0, SWF_COLOR_G (color) / 255.0,
      SWF_COLOR_B (color) / 255.0, SWF_COLOR_A (color) / 255.0);
}

unsigned int
swfdec_color_apply_transform (unsigned int in, const SwfdecColorTransform * trans)
{
  int r, g, b, a;

  r = SWF_COLOR_R (in);
  g = SWF_COLOR_G (in);
  b = SWF_COLOR_B (in);
  a = SWF_COLOR_A (in);

  SWFDEC_LOG ("in rgba %d,%d,%d,%d", r, g, b, a);

  r = (r * trans->ra >> 8) + trans->rb;
  g = (g * trans->ga >> 8) + trans->gb;
  b = (b * trans->ba >> 8) + trans->bb;
  a = (a * trans->aa >> 8) + trans->ab;

  r = CLAMP (r, 0, 255);
  g = CLAMP (g, 0, 255);
  b = CLAMP (b, 0, 255);
  a = CLAMP (a, 0, 255);

  SWFDEC_LOG ("out rgba %d,%d,%d,%d", r, g, b, a);

  return SWF_COLOR_COMBINE (r, g, b, a);
}

/**
 * swfdec_color_transform_init_identity:
 * @trans: a #SwfdecColorTransform
 *
 * Initializes the color transform so it doesn't transform any colors.
 **/
void
swfdec_color_transform_init_identity (SwfdecColorTransform * trans)
{
  g_return_if_fail (trans != NULL);
  
  trans->ra = 256;
  trans->ga = 256;
  trans->ba = 256;
  trans->aa = 256;
  trans->rb = 0;
  trans->gb = 0;
  trans->bb = 0;
  trans->ab = 0;
}

/**
 * swfdec_color_transform_init_color:
 * @trans: a #SwfdecColorTransform
 * @color: a #SwfdecColor to transform to
 *
 * Initializes this color transform so it results in exactly @color no matter 
 * the input.
 **/
void
swfdec_color_transform_init_color (SwfdecColorTransform *trans, SwfdecColor color)
{
  trans->ra = 0;
  trans->rb = SWF_COLOR_R (color);
  trans->ga = 0;
  trans->gb = SWF_COLOR_G (color);
  trans->ba = 0;
  trans->bb = SWF_COLOR_B (color);
  trans->aa = 0;
  trans->ab = SWF_COLOR_A (color);
}

/**
 * swfdec_color_transform_chain:
 * @dest: #SwfdecColorTransform to take the result
 * @last: a #SwfdecColorTransform
 * @first: a #SwfdecColorTransform
 *
 * Computes a color transform that has the same effect as if the color 
 * transforms @first and @last would have been applied one after another.
 **/
void
swfdec_color_transform_chain (SwfdecColorTransform *dest,
    const SwfdecColorTransform *last, const SwfdecColorTransform *first)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (last != NULL);
  g_return_if_fail (first != NULL);
  
  /* FIXME: CLAMP here? */
  dest->ra = last->ra * first->ra >> 8;
  dest->rb = (last->ra * first->rb >> 8) + last->rb;
  dest->ga = last->ga * first->ga >> 8;
  dest->gb = (last->ga * first->gb >> 8) + last->gb;
  dest->ba = last->ba * first->ba >> 8;
  dest->bb = (last->ba * first->bb >> 8) + last->bb;
  dest->aa = last->aa * first->aa >> 8;
  dest->ab = (last->aa * first->ab >> 8) + last->ab;
}

static void
swfdec_cairo_matrix_ensure_invertible (cairo_matrix_t *matrix)
{
  cairo_matrix_t tmp = *matrix;
  while (cairo_matrix_invert (&tmp)) {
    SWFDEC_WARNING ("matrix not invertible, adding epsilon to smallest member");
    /* add epsilon at point closest to zero */
#define EPSILON (1.0 / SWFDEC_FIXED_SCALE_FACTOR)
    if (ABS (matrix->xx) <= ABS (matrix->xy) && 
	ABS (matrix->xx) <= ABS (matrix->yx) &&
	ABS (matrix->xx) <= ABS (matrix->yy))
      matrix->xx += (matrix->xx >= 0) ? EPSILON : -EPSILON;
    else if (ABS (matrix->yy) <= ABS (matrix->xy) &&
	     ABS (matrix->yy) <= ABS (matrix->yx))
      matrix->yy += (matrix->yy >= 0) ? EPSILON : -EPSILON;
    else if (ABS (matrix->xy) <= ABS (matrix->yx))
      matrix->xy += (matrix->xy >= 0) ? EPSILON : -EPSILON;
    else
      matrix->yx += (matrix->yx >= 0) ? EPSILON : -EPSILON;
    tmp = *matrix;
  }
}

void
swfdec_transform_init_identity (SwfdecTransform *trans)
{
  trans->xx = SWFDEC_INT_TO_FIXED (1);
  trans->yy = SWFDEC_INT_TO_FIXED (1);
  trans->xy = 0;
  trans->yx = 0;
  trans->x0 = 0;
  trans->y0 = 0;
}

/**
 * swfdec_transform_to_matrix:
 * @matrix: a cairo matrix
 * @trans: a #SwfdecTransform
 *
 * Applies the swfdec transform to a cairo matrix.
 **/
void
swfdec_transform_to_matrix (cairo_matrix_t *matrix, const SwfdecTransform *trans)
{
  g_return_if_fail (matrix != NULL);
  g_return_if_fail (trans != NULL);

  matrix->xx = SWFDEC_FIXED_TO_DOUBLE (trans->xx);
  matrix->xy = SWFDEC_FIXED_TO_DOUBLE (trans->xy);
  matrix->yx = SWFDEC_FIXED_TO_DOUBLE (trans->yx);
  matrix->yy = SWFDEC_FIXED_TO_DOUBLE (trans->yy);
  matrix->x0 = trans->x0;
  matrix->y0 = trans->y0;
  swfdec_cairo_matrix_ensure_invertible (matrix);
}
