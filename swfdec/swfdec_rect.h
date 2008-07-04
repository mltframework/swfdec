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

#ifndef __SWFDEC_RECT_H__
#define __SWFDEC_RECT_H__

#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_rectangle.h>

G_BEGIN_DECLS

struct _SwfdecRect
{
  double x0;
  double y0;
  double x1;
  double y1;
};

void swfdec_rect_init_empty (SwfdecRect *rect);

gboolean swfdec_rect_intersect (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b);
void swfdec_rect_round (SwfdecRect *dest, SwfdecRect *src);
void swfdec_rect_union (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b);
void swfdec_rect_subtract (SwfdecRect *dest, const SwfdecRect *a, const SwfdecRect *b);
void swfdec_rect_scale (SwfdecRect *dest, const SwfdecRect *src, double factor);
gboolean swfdec_rect_is_empty (const SwfdecRect * a);
/* FIXME: rename to _contains_point and _contains instead of _inside? */
gboolean swfdec_rect_contains (const SwfdecRect *rect, double x, double y);
gboolean swfdec_rect_inside (const SwfdecRect *outer, const SwfdecRect *inner);
void swfdec_rect_transform (SwfdecRect *dest, const SwfdecRect *src, const cairo_matrix_t *matrix);

void swfdec_rectangle_init_rect (SwfdecRectangle *rectangle, const SwfdecRect *rect);

G_END_DECLS
#endif
