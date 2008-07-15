/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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
swfdec_color_apply_morph (SwfdecColor start, SwfdecColor end, guint ratio)
{
  guint r, g, b, a;
  guint start_ratio, end_ratio;

  g_assert (ratio < 65536);
  if (ratio == 0)
    return start;
  if (ratio == 65535)
    return end;
  start_ratio = 65535 - ratio;
  end_ratio = ratio;
  r = (SWFDEC_COLOR_R (start) * start_ratio + SWFDEC_COLOR_R (end) * end_ratio) / 65535;
  g = (SWFDEC_COLOR_G (start) * start_ratio + SWFDEC_COLOR_G (end) * end_ratio) / 65535;
  b = (SWFDEC_COLOR_B (start) * start_ratio + SWFDEC_COLOR_B (end) * end_ratio) / 65535;
  a = (SWFDEC_COLOR_A (start) * start_ratio + SWFDEC_COLOR_A (end) * end_ratio) / 65535;

  return SWFDEC_COLOR_COMBINE (r, g, b, a);
}

void 
swfdec_color_set_source (cairo_t *cr, SwfdecColor color)
{
  cairo_set_source_rgba (cr, 
      SWFDEC_COLOR_R (color) / 255.0, SWFDEC_COLOR_G (color) / 255.0,
      SWFDEC_COLOR_B (color) / 255.0, SWFDEC_COLOR_A (color) / 255.0);
}

SwfdecColor
swfdec_color_apply_transform_premultiplied (SwfdecColor in, 
    const SwfdecColorTransform * trans)
{
  int r, g, b, a, aold;

  if (trans->mask)
    return SWFDEC_COLOR_COMBINE (0, 0, 0, 255);

  aold = SWFDEC_COLOR_A (in);
  if (aold == 0)
    return 0;

  a = (aold * trans->aa >> 8) + trans->ab;
  a = CLAMP (a, 0, 255);

  r = SWFDEC_COLOR_R (in);
  g = SWFDEC_COLOR_G (in);
  b = SWFDEC_COLOR_B (in);
  r = (r * trans->ra * a / aold >> 8) + trans->rb * a / 255;
  r = CLAMP (r, 0, a);
  g = (g * trans->ga * a / aold >> 8) + trans->gb * a / 255;
  g = CLAMP (g, 0, a);
  b = (b * trans->ba * a / aold >> 8) + trans->bb * a / 255;
  b = CLAMP (b, 0, a);

  return SWFDEC_COLOR_COMBINE (r, g, b, a);
}

SwfdecColor
swfdec_color_apply_transform (SwfdecColor in, const SwfdecColorTransform * trans)
{
  int r, g, b, a;

  if (trans->mask)
    return SWFDEC_COLOR_COMBINE (0, 0, 0, 255);

  r = SWFDEC_COLOR_R (in);
  g = SWFDEC_COLOR_G (in);
  b = SWFDEC_COLOR_B (in);
  a = SWFDEC_COLOR_A (in);

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

  return SWFDEC_COLOR_COMBINE (r, g, b, a);
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
  
  trans->mask = FALSE;
  trans->ra = 256;
  trans->ga = 256;
  trans->ba = 256;
  trans->aa = 256;
  trans->rb = 0;
  trans->gb = 0;
  trans->bb = 0;
  trans->ab = 0;
}

void
swfdec_color_transform_init_mask (SwfdecColorTransform *trans)
{
  g_return_if_fail (trans != NULL);
  
  trans->mask = TRUE;
  /* don't init the other values so valgrind complains when they get accessed */
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
  trans->mask = FALSE;
  trans->ra = 0;
  trans->rb = SWFDEC_COLOR_R (color);
  trans->ga = 0;
  trans->gb = SWFDEC_COLOR_G (color);
  trans->ba = 0;
  trans->bb = SWFDEC_COLOR_B (color);
  trans->aa = 0;
  trans->ab = SWFDEC_COLOR_A (color);
}

gboolean
swfdec_color_transform_is_identity (const SwfdecColorTransform * trans)
{
  return trans->mask == FALSE && 
      trans->ra == 256 && trans->ga == 256 && trans->ba == 256 && trans->aa == 256 &&
      trans->rb == 0 && trans->gb == 0 && trans->bb == 0 && trans->ab == 0;
}

/**
 * swfdec_color_transform_is_alpha:
 * @trans: a color transform
 *
 * Checks if the color transform is an alpha transform. Note that the identity
 * transform is not considered an alpha transform.
 *
 * Returns: %TRUE if the color transform is an alpha transform
 **/
gboolean
swfdec_color_transform_is_alpha (const SwfdecColorTransform * trans)
{
  return trans->mask == FALSE && 
      trans->ra == 256 && trans->ga == 256 && trans->ba == 256 && trans->aa != 256 &&
      trans->rb == 0 && trans->gb == 0 && trans->bb == 0 && trans->ab == 0;
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
  g_return_if_fail (!last->mask);
  
  if (first->mask) {
    swfdec_color_transform_init_mask (dest);
    return;
  }
  dest->mask = FALSE;
  dest->ra = last->ra * first->ra >> 8;
  dest->rb = (last->ra * first->rb >> 8) + last->rb;
  dest->ga = last->ga * first->ga >> 8;
  dest->gb = (last->ga * first->gb >> 8) + last->gb;
  dest->ba = last->ba * first->ba >> 8;
  dest->bb = (last->ba * first->bb >> 8) + last->bb;
  dest->aa = last->aa * first->aa >> 8;
  dest->ab = (last->aa * first->ab >> 8) + last->ab;
}

void
swfdec_matrix_ensure_invertible (cairo_matrix_t *matrix, cairo_matrix_t *inverse)
{
  cairo_matrix_t tmp;
  
  g_return_if_fail (matrix != NULL);

  if (inverse == NULL)
    inverse = &tmp;
  
  g_assert (isfinite (matrix->xx) && isfinite (matrix->yx) && isfinite (matrix->xy) && isfinite (matrix->yy));
  *inverse = *matrix;
  while (cairo_matrix_invert (inverse)) {
    SWFDEC_INFO ("matrix not invertible, adding epsilon to smallest member");
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
    *inverse = *matrix;
  }
}

double
swfdec_matrix_get_xscale (const cairo_matrix_t *matrix)
{
  double alpha;

  if (matrix->xx) {
    alpha = atan2 (matrix->yx, matrix->xx);
    alpha = cos (alpha);
    return matrix->xx / alpha * 100;
  } else {
    return matrix->yx * 100;
  }
}

double
swfdec_matrix_get_yscale (const cairo_matrix_t *matrix)
{
  double alpha;

  if (matrix->yy) {
    alpha = atan2 (matrix->xy, matrix->yy);
    alpha = cos (alpha);
    return matrix->yy / alpha * 100;
  } else {
    return matrix->xy * 100;
  }
}

double
swfdec_matrix_get_rotation (const cairo_matrix_t *matrix)
{
  return atan2 (matrix->yx, matrix->xx) * 180 / G_PI;
}

void
swfdec_matrix_morph (cairo_matrix_t *dest, const cairo_matrix_t *start,
    const cairo_matrix_t *end, guint ratio)
{
  guint inv_ratio = 65535 - ratio;
  g_assert (ratio < 65536);

  if (ratio == 0) {
    *dest = *start;
    return;
  }
  if (ratio == 65535) {
    *dest = *end;
    return;
  }
  dest->xx = (start->xx * inv_ratio + end->xx * ratio) / 65535;
  dest->xy = (start->xy * inv_ratio + end->xy * ratio) / 65535;
  dest->yy = (start->yy * inv_ratio + end->yy * ratio) / 65535;
  dest->yx = (start->yx * inv_ratio + end->yx * ratio) / 65535;
  dest->x0 = (start->x0 * inv_ratio + end->x0 * ratio) / 65535;
  dest->y0 = (start->y0 * inv_ratio + end->y0 * ratio) / 65535;
}

