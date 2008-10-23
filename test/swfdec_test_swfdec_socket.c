/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_test_swfdec_socket.h"

/*** SwfdecTestSwfdecSocket ***/

G_DEFINE_TYPE (SwfdecTestSwfdecSocket, swfdec_test_swfdec_socket, SWFDEC_TYPE_SOCKET)

#define SWFDEC_TEST_SWFDEC_SOCKET_FROM_PLUGIN(x) \
  SWFDEC_TEST_SWFDEC_SOCKET ((gpointer) ((guint8 *) (x) - G_STRUCT_OFFSET (SwfdecTestSwfdecSocket, plugin)))

static void
swfdec_test_swfdec_socket_close (SwfdecStream *stream)
{
  SwfdecTestSwfdecSocket *test = SWFDEC_TEST_SWFDEC_SOCKET (stream);

  test->plugin.close (&test->plugin);
  g_free (test->plugin.host);
  test->plugin.host = NULL;
}

static void
swfdec_test_swfdec_socket_connect (SwfdecSocket *sock_, SwfdecPlayer *player, 
    const char *hostname, guint port)
{
  SwfdecTestSwfdecSocket *sock = SWFDEC_TEST_SWFDEC_SOCKET (sock_);
  SwfdecTestPlugin *plugin = g_object_get_data (G_OBJECT (player), "plugin");

  g_assert (plugin);
  sock->plugin.host = g_strdup (hostname);
  sock->plugin.port = port;
  plugin->request_socket (plugin, &sock->plugin);
  g_object_ref (sock);
  /* FIXME: allow testing this, too? */
  swfdec_stream_open (SWFDEC_STREAM (sock));
}

static void
swfdec_test_swfdec_socket_send (SwfdecSocket *sock, SwfdecBuffer *buffer)
{
  SwfdecTestSwfdecSocket *test = SWFDEC_TEST_SWFDEC_SOCKET (sock);

  test->plugin.send (&test->plugin, buffer->data, buffer->length);
  swfdec_buffer_unref (buffer);
}

static void
swfdec_test_swfdec_socket_class_init (SwfdecTestSwfdecSocketClass *klass)
{
  SwfdecStreamClass *stream_class = SWFDEC_STREAM_CLASS (klass);
  SwfdecSocketClass *socket_class = SWFDEC_SOCKET_CLASS (klass);

  stream_class->close = swfdec_test_swfdec_socket_close;

  socket_class->connect = swfdec_test_swfdec_socket_connect;
  socket_class->send = swfdec_test_swfdec_socket_send;
}

static void
swfdec_test_swfdec_socket_receive (SwfdecTestPluginSocket *plugin,
    unsigned char *data, unsigned long length)
{
  SwfdecTestSwfdecSocket *sock = SWFDEC_TEST_SWFDEC_SOCKET_FROM_PLUGIN (plugin);
  SwfdecBuffer *buffer;

  buffer = swfdec_buffer_new_for_data (g_memdup (data, length), length);
  swfdec_stream_push (SWFDEC_STREAM (sock), buffer);
}

static void
swfdec_test_swfdec_socket_finish (SwfdecTestPluginSocket *plugin, int error)
{
  SwfdecTestSwfdecSocket *sock = SWFDEC_TEST_SWFDEC_SOCKET_FROM_PLUGIN (plugin);

  if (error) {
    swfdec_stream_error (SWFDEC_STREAM (sock), "error %d", error);
  } else {
    swfdec_stream_close (SWFDEC_STREAM (sock));
  }
  g_object_unref (sock);
}

static void
swfdec_test_swfdec_socket_init (SwfdecTestSwfdecSocket *sock)
{
  sock->plugin.receive = swfdec_test_swfdec_socket_receive;
  sock->plugin.finish = swfdec_test_swfdec_socket_finish;
}

