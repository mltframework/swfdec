/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifndef __SWFDEC_RECTANGLE_H__
#define __SWFDEC_RECTANGLE_H__

#include <glib-object.h>

#define SWFDEC_TYPE_RECTANGLE swfdec_rectangle_get_type ()

typedef struct _SwfdecRectangle SwfdecRectangle;

struct _SwfdecRectangle {
  int		x;			/* x coordinate of top-left point */
  int		y;			/* y coordinate of top left point */
  int		width;			/* width of rectangle or 0 for empty */
  int		height;			/* height of rectangle or 0 for empty */
};

GType		swfdec_rectangle_get_type	    (void);

void		swfdec_rectangle_init_empty	    (SwfdecRectangle *		rectangle);

gboolean	swfdec_rectangle_is_empty	    (const SwfdecRectangle *	rectangle);

gboolean	swfdec_rectangle_intersect	    (SwfdecRectangle *		dest,
						     const SwfdecRectangle *	a,
						     const SwfdecRectangle *	b);
void		swfdec_rectangle_union		    (SwfdecRectangle *		dest,
						     const SwfdecRectangle *	a,
						     const SwfdecRectangle *	b);
gboolean	swfdec_rectangle_contains	    (const SwfdecRectangle *	container,
						     const SwfdecRectangle *	content);
gboolean	swfdec_rectangle_contains_point	    (const SwfdecRectangle *	rectangle,
						     int			x,
						     int			y);


#endif
