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

#ifndef _SWFDEC_AS_NUMBER_H_
#define _SWFDEC_AS_NUMBER_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsNumber SwfdecAsNumber;
typedef struct _SwfdecAsNumberClass SwfdecAsNumberClass;

#define SWFDEC_TYPE_AS_NUMBER                    (swfdec_as_number_get_type())
#define SWFDEC_IS_AS_NUMBER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_NUMBER))
#define SWFDEC_IS_AS_NUMBER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_NUMBER))
#define SWFDEC_AS_NUMBER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_NUMBER, SwfdecAsNumber))
#define SWFDEC_AS_NUMBER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_NUMBER, SwfdecAsNumberClass))
#define SWFDEC_AS_NUMBER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_NUMBER, SwfdecAsNumberClass))

struct _SwfdecAsNumber {
  SwfdecAsObject	object;

  double		number;		/* number represented by this number object */
};

struct _SwfdecAsNumberClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_as_number_get_type	(void);


G_END_DECLS
#endif
