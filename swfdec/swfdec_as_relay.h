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

#ifndef _SWFDEC_AS_RELAY_H_
#define _SWFDEC_AS_RELAY_H_

#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_gc_object.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsRelayClass SwfdecAsRelayClass;

#define SWFDEC_TYPE_AS_RELAY                    (swfdec_as_relay_get_type())
#define SWFDEC_IS_AS_RELAY(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_RELAY))
#define SWFDEC_IS_AS_RELAY_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_RELAY))
#define SWFDEC_AS_RELAY(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_RELAY, SwfdecAsRelay))
#define SWFDEC_AS_RELAY_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_RELAY, SwfdecAsRelayClass))
#define SWFDEC_AS_RELAY_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_RELAY, SwfdecAsRelayClass))

struct _SwfdecAsRelay {
  /*< protected >*/
  SwfdecGcObject	object;
  SwfdecAsObject *	relay;
};

struct _SwfdecAsRelayClass {
  /*< private >*/
  SwfdecGcObjectClass	object_class;
};

GType		swfdec_as_relay_get_type	(void);


gboolean	swfdec_as_relay_call		(SwfdecAsRelay *	relay,
						 const char *		name,
						 guint			argc,
						 SwfdecAsValue *	argv,
						 SwfdecAsValue *	return_value);


G_END_DECLS
#endif
