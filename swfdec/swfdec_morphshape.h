/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_MORPH_SHAPE_H_
#define _SWFDEC_MORPH_SHAPE_H_

#include <swfdec/swfdec_shape.h>

G_BEGIN_DECLS

typedef struct _SwfdecMorphShape SwfdecMorphShape;
typedef struct _SwfdecMorphShapeClass SwfdecMorphShapeClass;

#define SWFDEC_TYPE_MORPH_SHAPE                    (swfdec_morph_shape_get_type())
#define SWFDEC_IS_MORPH_SHAPE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_MORPH_SHAPE))
#define SWFDEC_IS_MORPH_SHAPE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_MORPH_SHAPE))
#define SWFDEC_MORPH_SHAPE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_MORPH_SHAPE, SwfdecMorphShape))
#define SWFDEC_MORPH_SHAPE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_MORPH_SHAPE, SwfdecMorphShapeClass))

struct _SwfdecMorphShape {
  SwfdecShape		shape;

  SwfdecRect		end_extents;	/* extents at end of morph (compare with graphic->extents for start) */
};

struct _SwfdecMorphShapeClass {
  SwfdecShapeClass	shape_class;
};

GType	swfdec_morph_shape_get_type	(void);

int	tag_define_morph_shape		(SwfdecSwfDecoder *	s,
					 guint			tag);


G_END_DECLS
#endif
