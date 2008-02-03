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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libsoup/soup-address.h>

#include "swfdec_test_socket.h"
#include "swfdec_test_buffer.h"
#include "swfdec_test_function.h"

static void
swfdec_test_throw (SwfdecAsContext *cx, const char *message, ...)
{
  SwfdecAsValue val;

  if (!swfdec_as_context_catch (cx, &val)) {
    va_list varargs;
    char *s;

    va_start (varargs, message);
    s = g_strdup_vprintf (message, varargs);
    va_end (varargs);

    /* FIXME: Throw a real object here? */
    SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_give_string (cx, s));
  }
  swfdec_as_context_throw (cx, &val);
}

/*** SWFDEC_TEST_SOCKET ***/

G_DEFINE_TYPE (SwfdecTestSocket, swfdec_test_socket, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_test_socket_dispose (GObject *object)
{
  SwfdecTestSocket *sock = SWFDEC_TEST_SOCKET (object);

  if (sock->socket) {
    g_object_unref (sock->socket);
    sock->socket = NULL;
  }
  if (sock->context) {
    g_main_context_unref (sock->context);
    sock->context = NULL;
  }
  g_slist_foreach (sock->connections, (GFunc) g_object_unref, NULL);
  g_slist_free (sock->connections);
  sock->connections = NULL;

  G_OBJECT_CLASS (swfdec_test_socket_parent_class)->dispose (object);
}

static void
swfdec_test_socket_class_init (SwfdecTestSocketClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_test_socket_dispose;
}

static void
swfdec_test_socket_init (SwfdecTestSocket *sock)
{
  sock->context = g_main_context_new ();
}

/*** AS CODE ***/

#define swfdec_test_socket_process(sock) do { \
  g_usleep (1000); \
} while (g_main_context_iteration ((sock)->context, FALSE))

static void
swfdec_test_socket_new_connection (SoupSocket *server, SoupSocket *conn, SwfdecTestSocket *sock)
{
  g_object_ref (conn);
  sock->connections = g_slist_append (sock->connections, conn);
}

static void
swfdec_test_socket_attach (SwfdecTestSocket *sock, SoupSocket *ssock)
{
  g_assert (sock->socket == NULL);

  sock->socket = ssock;
  g_signal_connect (ssock, "new-connection", 
      G_CALLBACK (swfdec_test_socket_new_connection), sock);
}

SWFDEC_TEST_FUNCTION ("Socket_getConnection", swfdec_test_socket_getConnection, 0)
void
swfdec_test_socket_getConnection (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;
  SwfdecAsObject *new;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "");

  if (!sock->listening) {
    swfdec_test_throw (cx, "only server sockets may call getConnection");
    return;
  }
  swfdec_test_socket_process (sock);
  if (sock->connections == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (retval);
    return;
  }

  if (!swfdec_as_context_use_mem (cx, sizeof (SwfdecTestSocket)))
    return;

  new = g_object_new (SWFDEC_TYPE_TEST_SOCKET, NULL);
  swfdec_as_object_add (new, cx, sizeof (SwfdecTestSocket));
  swfdec_as_object_get_variable (cx->global, 
      swfdec_as_context_get_string (cx, "Socket"), &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val))
    swfdec_as_object_set_constructor (new, SWFDEC_AS_VALUE_GET_OBJECT (&val));

  swfdec_test_socket_attach (SWFDEC_TEST_SOCKET (new), sock->connections->data);
  sock->connections = g_slist_remove (sock->connections, sock->connections->data);
  SWFDEC_AS_VALUE_SET_OBJECT (retval, new);
}

SWFDEC_TEST_FUNCTION ("Socket_send", swfdec_test_socket_send, 0)
void
swfdec_test_socket_send (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;
  SwfdecBuffer *buffer;
  GError *error = NULL;
  gsize written;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "");

  if (sock->listening) {
    swfdec_test_throw (cx, "server sockets may not call send");
    return;
  }
  swfdec_test_socket_process (sock);
  buffer = swfdec_test_buffer_from_args (cx, argc, argv);
  if (soup_socket_write (sock->socket, buffer->data, buffer->length, &written,
	NULL, &error) != SOUP_SOCKET_OK) {
    swfdec_test_throw (cx, "%s", error->message);
    g_error_free (error);
  } else if (buffer->length != written) {
    swfdec_test_throw (cx, "only wrote %u bytes of %u", written, buffer->length);
  }
  swfdec_buffer_unref (buffer);
  swfdec_test_socket_process (sock);
}

SWFDEC_TEST_FUNCTION ("Socket_receive", swfdec_test_socket_receive, 0)
void
swfdec_test_socket_receive (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;
  SwfdecBuffer *buffer;
  GError *error = NULL;
  gsize written;
  guint len;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "|i", &len);

  if (sock->listening) {
    swfdec_test_throw (cx, "server sockets may not call receive");
    return;
  }
  swfdec_test_socket_process (sock);
  if (len > 0) {
    buffer = swfdec_buffer_new_and_alloc (len);
    if (soup_socket_read (sock->socket, buffer->data, buffer->length, &len,
	  NULL, &error) != SOUP_SOCKET_OK) {
      swfdec_test_throw (cx, "%s", error->message);
      g_error_free (error);
      swfdec_buffer_unref (buffer);
      return;
    } else if (buffer->length != len) {
      swfdec_test_throw (cx, "only read %u bytes of %u", written, buffer->length);
      swfdec_buffer_unref (buffer);
      return;
    }
  } else {
    SwfdecBufferQueue *queue = swfdec_buffer_queue_new ();
    SoupSocketIOStatus status = SOUP_SOCKET_OK;
    while (status == SOUP_SOCKET_OK) {
      buffer = swfdec_buffer_new_and_alloc (128);
      status = soup_socket_read (sock->socket, buffer->data, 128, &
	  buffer->length, NULL, &error);
      if (status != SOUP_SOCKET_OK && status != SOUP_SOCKET_WOULD_BLOCK) {
	swfdec_test_throw (cx, "%s", error->message);
	g_error_free (error);
	swfdec_buffer_unref (buffer);
	swfdec_buffer_queue_unref (queue);
	return;
      }
      swfdec_buffer_queue_push (queue, buffer);
    }
    buffer = swfdec_buffer_queue_pull (queue, swfdec_buffer_queue_get_depth (queue));
    swfdec_buffer_queue_unref (queue);
  }
  SWFDEC_AS_VALUE_SET_OBJECT (retval, swfdec_test_buffer_new (cx, buffer));
}

SWFDEC_TEST_FUNCTION ("Socket_close", swfdec_test_socket_close, 0)
void
swfdec_test_socket_close (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  swfdec_test_throw (cx, "implement");
}

SWFDEC_TEST_FUNCTION ("Socket", swfdec_test_socket_create, swfdec_test_socket_get_type)
void
swfdec_test_socket_create (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;
  SoupSocket *ssock;
  int port;
  SoupAddress *addr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "i", &port);

  if (!swfdec_as_context_is_constructing (cx))
    return;

  sock->listening = TRUE;
  /* FIXME: throw here? */
  if (port < 1024 || port >= 65535) {
    swfdec_test_throw (cx, "invalid port %d", port);
    return;
  }

  addr = soup_address_new_any (SOUP_ADDRESS_FAMILY_IPV4, port);
  ssock = soup_socket_new (
      SOUP_SOCKET_FLAG_NONBLOCKING, TRUE, 
      SOUP_SOCKET_ASYNC_CONTEXT, sock->context,
      SOUP_SOCKET_LOCAL_ADDRESS, addr,
      NULL);
  g_object_unref (addr);
  swfdec_test_socket_attach (sock, ssock);
  if (!soup_socket_listen (ssock)) {
    swfdec_test_throw (cx, "failed to listen");
    return;
  }
}

