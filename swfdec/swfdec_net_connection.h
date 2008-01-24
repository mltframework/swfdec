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

#ifndef _SWFDEC_NET_CONNECTION_H_
#define _SWFDEC_NET_CONNECTION_H_

#include <swfdec/swfdec_as_object.h>

G_BEGIN_DECLS


typedef struct _SwfdecNetConnection SwfdecNetConnection;
typedef struct _SwfdecNetConnectionClass SwfdecNetConnectionClass;

#define SWFDEC_TYPE_NET_CONNECTION                    (swfdec_net_connection_get_type())
#define SWFDEC_IS_NET_CONNECTION(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_NET_CONNECTION))
#define SWFDEC_IS_NET_CONNECTION_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_NET_CONNECTION))
#define SWFDEC_NET_CONNECTION(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_NET_CONNECTION, SwfdecNetConnection))
#define SWFDEC_NET_CONNECTION_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_NET_CONNECTION, SwfdecNetConnectionClass))
#define SWFDEC_NET_CONNECTION_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_NET_CONNECTION, SwfdecNetConnectionClass))

struct _SwfdecNetConnection {
  SwfdecAsObject	object;

  char *		url;		/* url for this net_connection or NULL for none */
};

struct _SwfdecNetConnectionClass {
  SwfdecAsObjectClass	object_class;
};

GType			swfdec_net_connection_get_type	(void);

SwfdecNetConnection *	swfdec_net_connection_new	(SwfdecAsContext *	context);

void			swfdec_net_connection_connect	(SwfdecNetConnection *	conn,
							 const char *		url);


G_END_DECLS
#endif
