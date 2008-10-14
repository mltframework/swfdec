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

#ifndef _SWFDEC_EVENT_DISPATCHER_H_
#define _SWFDEC_EVENT_DISPATCHER_H_

#include <swfdec/swfdec_gc_object.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS

//typedef struct _SwfdecEventDispatcher SwfdecEventDispatcher;
typedef struct _SwfdecEventDispatcherClass SwfdecEventDispatcherClass;

#define SWFDEC_TYPE_EVENT_DISPATCHER                    (swfdec_event_dispatcher_get_type())
#define SWFDEC_IS_EVENT_DISPATCHER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_EVENT_DISPATCHER))
#define SWFDEC_IS_EVENT_DISPATCHER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_EVENT_DISPATCHER))
#define SWFDEC_EVENT_DISPATCHER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_EVENT_DISPATCHER, SwfdecEventDispatcher))
#define SWFDEC_EVENT_DISPATCHER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_EVENT_DISPATCHER, SwfdecEventDispatcherClass))
#define SWFDEC_EVENT_DISPATCHER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_EVENT_DISPATCHER, SwfdecEventDispatcherClass))

struct _SwfdecEventDispatcher
{
  SwfdecGcObject	object;
};

struct _SwfdecEventDispatcherClass
{
  SwfdecGcObjectClass	object_class;
};

GType			swfdec_event_dispatcher_get_type	(void);


G_END_DECLS
#endif
