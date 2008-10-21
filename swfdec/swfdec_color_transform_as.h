/* Swfdec
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_COLOR_TRANSFORM_H_
#define _SWFDEC_COLOR_TRANSFORM_H_

#include <swfdec/swfdec_as_relay.h>
#include <swfdec/swfdec_color.h>

G_BEGIN_DECLS

typedef struct _SwfdecColorTransformAs SwfdecColorTransformAs;
typedef struct _SwfdecColorTransformAsClass SwfdecColorTransformAsClass;

#define SWFDEC_TYPE_COLOR_TRANSFORM_AS                    (swfdec_color_transform_as_get_type())
#define SWFDEC_IS_COLOR_TRANSFORM_AS(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_COLOR_TRANSFORM_AS))
#define SWFDEC_IS_COLOR_TRANSFORM_AS_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_COLOR_TRANSFORM_AS))
#define SWFDEC_COLOR_TRANSFORM_AS(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_COLOR_TRANSFORM_AS, SwfdecColorTransformAs))
#define SWFDEC_COLOR_TRANSFORM_AS_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_COLOR_TRANSFORM_AS, SwfdecColorTransformAsClass))
#define SWFDEC_COLOR_TRANSFORM_AS_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_COLOR_TRANSFORM_AS, SwfdecColorTransformAsClass))

struct _SwfdecColorTransformAs {
  SwfdecAsRelay		relay;

  double		ra, rb, ga, gb, ba, bb, aa, ab;
};

struct _SwfdecColorTransformAsClass {
  SwfdecAsRelayClass	relay_class;
};

GType			swfdec_color_transform_as_get_type		(void);

SwfdecColorTransformAs *swfdec_color_transform_as_new_from_transform	(SwfdecAsContext *		context,
									 const SwfdecColorTransform *	transform);
void			swfdec_color_transform_get_transform		(SwfdecColorTransformAs *	trans,
									 SwfdecColorTransform *		ctrans);

G_END_DECLS
#endif
