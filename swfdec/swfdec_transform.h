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

#ifndef _SWFDEC_AS_ARRAY_H_
#define _SWFDEC_AS_ARRAY_H_

#include <swfdec/swfdec_movie.h>
#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecTransform SwfdecTransform;
typedef struct _SwfdecTransformClass SwfdecTransformClass;

#define SWFDEC_TYPE_TRANSFORM                    (swfdec_transform_get_type())
#define SWFDEC_IS_TRANSFORM(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TRANSFORM))
#define SWFDEC_IS_TRANSFORM_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TRANSFORM))
#define SWFDEC_TRANSFORM(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TRANSFORM, SwfdecTransform))
#define SWFDEC_TRANSFORM_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TRANSFORM, SwfdecTransformClass))
#define SWFDEC_TRANSFORM_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TRANSFORM, SwfdecTransformClass))

struct _SwfdecTransform {
  SwfdecAsObject	object;

  SwfdecMovie *		target;
};

struct _SwfdecTransformClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_transform_get_type	(void);


G_END_DECLS
#endif
