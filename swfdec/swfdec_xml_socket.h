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

#ifndef _SWFDEC_XML_SOCKET_H_
#define _SWFDEC_XML_SOCKET_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_sandbox.h>

G_BEGIN_DECLS


typedef struct _SwfdecXmlSocket SwfdecXmlSocket;
typedef struct _SwfdecXmlSocketClass SwfdecXmlSocketClass;

#define SWFDEC_TYPE_XML_SOCKET                    (swfdec_xml_socket_get_type())
#define SWFDEC_IS_XML_SOCKET(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_XML_SOCKET))
#define SWFDEC_IS_XML_SOCKET_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_XML_SOCKET))
#define SWFDEC_XML_SOCKET(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_XML_SOCKET, SwfdecXmlSocket))
#define SWFDEC_XML_SOCKET_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_XML_SOCKET, SwfdecXmlSocketClass))
#define SWFDEC_XML_SOCKET_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_XML_SOCKET, SwfdecXmlSocketClass))

struct _SwfdecXmlSocket {
  SwfdecAsObject	object;

  SwfdecSocket *	socket;		/* the socket in use */
  SwfdecSandbox *	sandbox;	/* the sandbox we run in */
  gboolean		open;		/* the socket has been opened already */
  SwfdecBufferQueue *	queue;		/* everything that belongs to the same string */
  SwfdecAsObject *	target;		/* target object we call out to */
  gboolean		target_owner;	/* TRUE if we own the target */
};

struct _SwfdecXmlSocketClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_xml_socket_get_type	(void);


G_END_DECLS
#endif
