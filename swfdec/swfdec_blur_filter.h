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

#ifndef _SWFDEC_BLUR_FILTER_H_
#define _SWFDEC_BLUR_FILTER_H_

#include <swfdec/swfdec_filter.h>

G_BEGIN_DECLS


typedef struct _SwfdecBlurFilter SwfdecBlurFilter;
typedef struct _SwfdecBlurFilterClass SwfdecBlurFilterClass;

#define SWFDEC_TYPE_BLUR_FILTER                    (swfdec_blur_filter_get_type())
#define SWFDEC_IS_BLUR_FILTER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_BLUR_FILTER))
#define SWFDEC_IS_BLUR_FILTER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_BLUR_FILTER))
#define SWFDEC_BLUR_FILTER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_BLUR_FILTER, SwfdecBlurFilter))
#define SWFDEC_BLUR_FILTER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_BLUR_FILTER, SwfdecBlurFilterClass))
#define SWFDEC_BLUR_FILTER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_BLUR_FILTER, SwfdecBlurFilterClass))

struct _SwfdecBlurFilter {
  SwfdecFilter		filter;

  double		x;		/* blur in horizontal direction */
  double		y;		/* blur in vertical direction */
  guint			quality;	/* number of passes */
};

struct _SwfdecBlurFilterClass {
  SwfdecFilterClass	filter_class;
};

GType			swfdec_blur_filter_get_type	(void);

void			swfdec_blur_filter_invalidate	(SwfdecBlurFilter *	filter);


G_END_DECLS
#endif
