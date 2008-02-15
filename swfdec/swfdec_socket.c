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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_socket.h"
#include "swfdec_loader_internal.h"

/*** GTK-DOC ***/

/**
 * SECTION:SwfdecSocket
 * @title: SwfdecSocket
 * @short_description: object used for network connections
 *
 * SwfdecSockets are used to implement TCP streams. These are for example used 
 * to implement RTMP support or the XMLSocket script class. Backends are 
 * supposed to provide an implementation for this, as by default this class is
 * unimplemented. However, swfdec-gtk or other convenience libraries provide
 * a socket implementation by default. 
 *
 * The socket implementation used by a #SwfdecPlayer can be set with the 
 * SwfdecPlayer:socket-type property.
 */

/**
 * SwfdecSocket:
 *
 * This is the base object used for providing input. It is abstract, create a 
 * subclass to provide your own socket implementation. All members are 
 * considered private.
 */

/**
 * SwfdecSocketClass:
 * @connect: Connect the given newly created socket to the given hostname and 
 *           port. If you encounter an error, call swfdec_stream_error(), but 
 *           still make sure the socket object does not break.
 * @send: Called to send data down the given socket. This function will only be
 *        called when the socket is open. You get passed a reference to the 
 *        buffer, so it is your responsibility to call swfdec_buffer_unref() on
 *        it when you are done with it.
 *
 * This is the socket class. When you create a subclass, you need to implement 
 * the functions listed above.
 */

/*** SWFDEC_SOCKET ***/

G_DEFINE_TYPE (SwfdecSocket, swfdec_socket, SWFDEC_TYPE_STREAM)

static void
swfdec_socket_do_connect (SwfdecSocket *socket, SwfdecPlayer *player,
    const char *hostname, guint port)
{
  swfdec_stream_error (SWFDEC_STREAM (socket), "no socket implementation exists");
}

static void
swfdec_socket_do_send (SwfdecSocket *socket, SwfdecBuffer *buffer)
{
  swfdec_buffer_unref (buffer);
}

static const char *
swfdec_socket_describe (SwfdecStream *stream)
{
  /* FIXME: add host/port */
  return G_OBJECT_TYPE_NAME (stream);
}

static void
swfdec_socket_class_init (SwfdecSocketClass *klass)
{
  SwfdecStreamClass *stream_class = SWFDEC_STREAM_CLASS (klass);

  stream_class->describe = swfdec_socket_describe;

  klass->connect = swfdec_socket_do_connect;
  klass->send = swfdec_socket_do_send;
}

static void
swfdec_socket_init (SwfdecSocket *socket)
{
}

/**
 * swfdec_socket_send:
 * @sock: a #SwfdecSocket
 * @buffer: data to send to the stream
 *
 * Pushes the given @buffer down the stream.
 **/
void
swfdec_socket_send (SwfdecSocket *sock, SwfdecBuffer *buffer)
{
  SwfdecSocketClass *klass;

  g_return_if_fail (SWFDEC_IS_SOCKET (sock));
  g_return_if_fail (swfdec_stream_is_open (SWFDEC_STREAM (sock)));
  g_return_if_fail (buffer != NULL);

  klass = SWFDEC_SOCKET_GET_CLASS (sock);
  klass->send (sock, buffer);
}

