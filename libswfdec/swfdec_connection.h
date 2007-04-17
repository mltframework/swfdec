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

#ifndef _SWFDEC_CONNECTION_H_
#define _SWFDEC_CONNECTION_H_

#include <libswfdec/swfdec_as_object.h>

G_BEGIN_DECLS


typedef struct _SwfdecConnection SwfdecConnection;
typedef struct _SwfdecConnectionClass SwfdecConnectionClass;

#define SWFDEC_TYPE_CONNECTION                    (swfdec_connection_get_type())
#define SWFDEC_IS_CONNECTION(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_CONNECTION))
#define SWFDEC_IS_CONNECTION_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_CONNECTION))
#define SWFDEC_CONNECTION(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_CONNECTION, SwfdecConnection))
#define SWFDEC_CONNECTION_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_CONNECTION, SwfdecConnectionClass))
#define SWFDEC_CONNECTION_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_CONNECTION, SwfdecConnectionClass))

struct _SwfdecConnection {
  SwfdecAsObject	object;

  char *		url;		/* url for this connection or NULL for none */
};

struct _SwfdecConnectionClass {
  SwfdecAsObjectClass	object_class;
};

GType			swfdec_connection_get_type	(void);

SwfdecConnection *	swfdec_connection_new		(SwfdecAsContext *	context);

void			swfdec_connection_connect	(SwfdecConnection *	conn,
							 const char *		url);


G_END_DECLS
#endif
