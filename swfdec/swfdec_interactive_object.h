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

#ifndef _SWFDEC_INTERACTIVE_OBJECT_H_
#define _SWFDEC_INTERACTIVE_OBJECT_H_

#include <swfdec/swfdec_display_object.h>

G_BEGIN_DECLS

//typedef struct _SwfdecInteractiveObject SwfdecInteractiveObject;
typedef struct _SwfdecInteractiveObjectClass SwfdecInteractiveObjectClass;

#define SWFDEC_TYPE_INTERACTIVE_OBJECT                    (swfdec_interactive_object_get_type())
#define SWFDEC_IS_INTERACTIVE_OBJECT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_INTERACTIVE_OBJECT))
#define SWFDEC_IS_INTERACTIVE_OBJECT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_INTERACTIVE_OBJECT))
#define SWFDEC_INTERACTIVE_OBJECT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_INTERACTIVE_OBJECT, SwfdecInteractiveObject))
#define SWFDEC_INTERACTIVE_OBJECT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_INTERACTIVE_OBJECT, SwfdecInteractiveObjectClass))
#define SWFDEC_INTERACTIVE_OBJECT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_INTERACTIVE_OBJECT, SwfdecInteractiveObjectClass))

struct _SwfdecInteractiveObject
{
  SwfdecDisplayObject		display_object;
};

struct _SwfdecInteractiveObjectClass
{
  SwfdecDisplayObjectClass	display_object_class;
};

GType			swfdec_interactive_object_get_type	(void);


G_END_DECLS
#endif
