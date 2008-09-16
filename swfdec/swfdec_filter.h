/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_FILTER_H_
#define _SWFDEC_FILTER_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecFilterClass SwfdecFilterClass;

#define SWFDEC_TYPE_FILTER                    (swfdec_filter_get_type())
#define SWFDEC_IS_FILTER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_FILTER))
#define SWFDEC_IS_FILTER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_FILTER))
#define SWFDEC_FILTER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_FILTER, SwfdecFilter))
#define SWFDEC_FILTER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_FILTER, SwfdecFilterClass))
#define SWFDEC_FILTER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_FILTER, SwfdecFilterClass))

struct _SwfdecFilter {
  SwfdecAsObject	object;
};

struct _SwfdecFilterClass {
  SwfdecAsObjectClass	object_class;

  void			(* clone)		(SwfdecFilter *		dest,
						 SwfdecFilter *		source);
  void			(* get_rectangle)	(SwfdecFilter *		filter,
						 SwfdecRectangle *	dest,
						 double			xscale,
						 double			yscale,
						 const SwfdecRectangle *source);
  cairo_pattern_t *	(* apply)		(SwfdecFilter *		filter,
						 cairo_pattern_t *	pattern,
						 double			xscale,
						 double			yscale,
						 const SwfdecRectangle *rect);
};

GType			swfdec_filter_get_type	(void);

SwfdecFilter *		swfdec_filter_clone		(SwfdecFilter *		filter);
cairo_pattern_t *	swfdec_filter_apply		(SwfdecFilter *		filter,
							 cairo_pattern_t *	pattern,
							 double			xscale,
							 double			yscale,
							 const SwfdecRectangle *source);
void			swfdec_filter_get_rectangle	(SwfdecFilter *		filter,
							 SwfdecRectangle *	dest,
							 double			xscale,
							 double			yscale,
							 const SwfdecRectangle *source);

GSList *		swfdec_filter_parse		(SwfdecPlayer *		player,
							 SwfdecBits *		bits);
void			swfdec_filter_skip		(SwfdecBits *		bits);
						

G_END_DECLS
#endif
