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
#include "swfdec_rect.h"

void 
swfdec_rect_init_empty (SwfdecRect *rect)
{
  g_return_if_fail (rect != NULL);

  rect->x0 = 0;
  rect->y0 = 0;
  rect->x1 = 0;
  rect->y1 = 0;
}

/**
 * swfdec_rect_round:
 * @dest: pointer to rect that will take the result
 * @src: #SwfdecRect that should be rounded
 *
 * Puts the smallest rectangle in @dest that includes @src but only
 * contains integer numbers
 **/
void
swfdec_rect_round (SwfdecRect *dest, SwfdecRect *src)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (src != NULL);

  if (swfdec_rect_is_empty (src)) {
    swfdec_rect_init_empty (dest);
    return;
  }
  dest->x0 = floor (src->x0);
  dest->y0 = floor (src->y0);
  dest->x1 = ceil (src->x1);
  dest->y1 = ceil (src->y1);
}

gboolean
swfdec_rect_intersect (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b)
{
  SwfdecRect tmp;

  g_return_val_if_fail (a != NULL, FALSE);
  g_return_val_if_fail (b != NULL, FALSE);
  if (dest == NULL)
    dest = &tmp;

  dest->x0 = MAX (a->x0, b->x0);
  dest->y0 = MAX (a->y0, b->y0);
  dest->x1 = MIN (a->x1, b->x1);
  dest->y1 = MIN (a->y1, b->y1);

  return !swfdec_rect_is_empty (dest);
}

/**
 * swfdec_rect_union:
 * @dest: destination rectangle
 * @a: first source rectangle, may be emtpy
 * @b: second source rectangle, may be empty
 *
 * Stores the union of @a and @b into @dest. The union is the smallest 
 * rectangle that includes both source rectangles. @a, @b and @dest may point 
 * to the same rectangle.
 **/
void
swfdec_rect_union (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (a != NULL);
  g_return_if_fail (b != NULL);

  if (swfdec_rect_is_empty (a)) {
    *dest = *b;
  } else if (swfdec_rect_is_empty (b)) {
    *dest = *a;
  } else {
    dest->x0 = MIN (a->x0, b->x0);
    dest->y0 = MIN (a->y0, b->y0);
    dest->x1 = MAX (a->x1, b->x1);
    dest->y1 = MAX (a->y1, b->y1);
  }
}

void
swfdec_rect_subtract (SwfdecRect *dest, const SwfdecRect *a, const SwfdecRect *b)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (a != NULL);
  g_return_if_fail (b != NULL);

  /* FIXME: improve this */
  if (swfdec_rect_is_empty (a)) {
    swfdec_rect_init_empty (dest);
  } else if (swfdec_rect_is_empty (b)) {
    *dest = *a;
  } else if (b->x0 <= a->x0 && b->x1 >= a->x1 &&
             b->y0 <= a->y0 && b->y1 >= a->y1) {
    swfdec_rect_init_empty (dest);
  } else {
    *dest = *a;
  }
}

void 
swfdec_rect_scale (SwfdecRect *dest, const SwfdecRect *src, double factor)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (src != NULL);

  dest->x0 = src->x0 * factor;
  dest->x1 = src->x1 * factor;
  dest->y0 = src->y0 * factor;
  dest->y1 = src->y1 * factor;
}

gboolean
swfdec_rect_is_empty (const SwfdecRect * a)
{
  return (a->x1 <= a->x0) || (a->y1 <= a->y0);
}

gboolean 
swfdec_rect_contains (const SwfdecRect *rect, double x, double y)
{
  return x >= rect->x0 &&
    x < rect->x1 &&
    y >= rect->y0 &&
    y < rect->y1;
}

/**
 * swfdec_rect_transform:
 * @dest: destination rectangle
 * @src: source rectangle
 * @matrix: matrix to apply to source
 *
 * Computes a rectangle that completely encloses the area that results from 
 * applying @matrix to @src.
 **/
void
swfdec_rect_transform (SwfdecRect *dest, const SwfdecRect *src, const cairo_matrix_t *matrix)
{
  SwfdecRect tmp, tmp2;

  g_return_if_fail (dest != NULL);
  g_return_if_fail (src != NULL);
  g_return_if_fail (matrix != NULL);

  tmp = *src;
  tmp2 = *src;
  cairo_matrix_transform_point (matrix, &tmp.x0, &tmp.y0);
  cairo_matrix_transform_point (matrix, &tmp.x1, &tmp.y1);
  cairo_matrix_transform_point (matrix, &tmp2.x0, &tmp2.y1);
  cairo_matrix_transform_point (matrix, &tmp2.x1, &tmp2.y0);

  dest->x0 = MIN (MIN (tmp.x0, tmp.x1), MIN (tmp2.x0, tmp2.x1));
  dest->y0 = MIN (MIN (tmp.y0, tmp.y1), MIN (tmp2.y0, tmp2.y1));
  dest->x1 = MAX (MAX (tmp.x0, tmp.x1), MAX (tmp2.x0, tmp2.x1));
  dest->y1 = MAX (MAX (tmp.y0, tmp.y1), MAX (tmp2.y0, tmp2.y1));
}

/**
 * swfdec_rect_inside:
 * @outer: the supposed outer rectangle
 * @inner: the supposed inner rectangle
 *
 * Checks if @outer completely includes the rectangle specified by @inner.
 * If both rectangles are empty, TRUE is returned.
 *
 * Returns: TRUE if @outer includes @inner, FALSE otherwise
 **/
gboolean
swfdec_rect_inside (const SwfdecRect *outer, const SwfdecRect *inner)
{
  /* empty includes empty */
  if (swfdec_rect_is_empty (inner))
    return TRUE;
  /* if outer is empty, below will return FALSE */
  return outer->x0 <= inner->x0 &&
	 outer->y0 <= inner->y0 &&
	 outer->x1 >= inner->x1 &&
	 outer->y1 >= inner->y1;
}

void 
swfdec_rectangle_init_rect (SwfdecRectangle *rectangle, const SwfdecRect *rect)
{
  g_return_if_fail (rectangle != NULL);
  g_return_if_fail (rect != NULL);

  rectangle->x = floor (rect->x0);
  rectangle->y = floor (rect->y0);
  rectangle->width = ceil (rect->x1) - rectangle->x;
  rectangle->height = ceil (rect->y1) - rectangle->y;
}

