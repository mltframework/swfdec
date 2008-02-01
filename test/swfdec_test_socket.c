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

SWFDEC_TEST_FUNCTION ("Socket_process", swfdec_test_socket_process, 0)
void
swfdec_test_socket_process (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;
  
  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &socket, "");

  while (g_main_context_iteration (sock->context, FALSE));
}

static void swfdec_test_socket_attach (SwfdecTestSocket *sock, SoupSocket *ssock);

static void
swfdec_test_socket_new_connection (SoupSocket *server, SoupSocket *conn, SwfdecTestSocket *sock)
{
  SwfdecAsContext *context = SWFDEC_AS_OBJECT (sock)->context;
  SwfdecAsValue val;
  SwfdecAsObject *new;

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecTestSocket)))
    return;

  new = g_object_new (SWFDEC_TYPE_TEST_SOCKET, NULL);
  swfdec_as_object_add (new, context, sizeof (SwfdecTestSocket));
  swfdec_as_object_get_variable (context->global, 
      swfdec_as_context_get_string (context, "Socket"), &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val))
    swfdec_as_object_set_constructor (new, SWFDEC_AS_VALUE_GET_OBJECT (&val));

  g_object_ref (conn);
  swfdec_test_socket_attach (SWFDEC_TEST_SOCKET (new), conn);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, new);
  swfdec_as_object_call (SWFDEC_AS_OBJECT (sock), 
      swfdec_as_context_get_string (context, "onNewConnection"),
      1, &val, NULL);
}

static void
swfdec_test_socket_attach (SwfdecTestSocket *sock, SoupSocket *ssock)
{
  g_assert (sock->socket == NULL);

  sock->socket = ssock;
  g_signal_connect (ssock, "new-connection", 
      G_CALLBACK (swfdec_test_socket_new_connection), sock);
}

SWFDEC_TEST_FUNCTION ("Socket", swfdec_test_socket_create, swfdec_test_socket_get_type)
void
swfdec_test_socket_create (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;
  int port;
  SoupAddress *addr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "i", &port);

  /* FIXME: throw here? */
  if (port < 1024 || port >= 65535) {
    swfdec_test_throw (cx, "invalid port %d", port);
    return;
  }

  addr = soup_address_new_any (SOUP_ADDRESS_FAMILY_IPV4, port);
  sock->socket = soup_socket_new (
      SOUP_SOCKET_FLAG_NONBLOCKING, TRUE, 
      SOUP_SOCKET_ASYNC_CONTEXT, sock->context,
      SOUP_SOCKET_LOCAL_ADDRESS, addr,
      NULL);
  g_object_unref (addr);
  if (!soup_socket_listen (sock->socket)) {
    swfdec_test_throw (cx, "failed to listen");
    return;
  }
}

