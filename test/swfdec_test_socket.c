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
swfdec_test_socket_do_close (SwfdecTestSocket *sock, gboolean success)
{
  if (sock->plugin == NULL)
    return;

  sock->plugin->finish (sock->plugin, success ? 0 : 1);
  sock->plugin = NULL;

  if (sock->test->pending_sockets &&
      sock->test->pending_sockets->data == sock) {
    sock->test->pending_sockets = sock->test->pending_sockets->prev;
  }
  sock->test->sockets = g_list_remove (sock->test->sockets, sock);
  sock->test = NULL;
}

static void
swfdec_test_socket_dispose (GObject *object)
{
  SwfdecTestSocket *sock = SWFDEC_TEST_SOCKET (object);

  swfdec_test_socket_do_close (sock, FALSE);
  swfdec_buffer_queue_unref (sock->receive_queue);
  sock->receive_queue = NULL;

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
  sock->receive_queue = swfdec_buffer_queue_new ();
}

/*** AS CODE ***/

SWFDEC_TEST_FUNCTION ("Socket_send", swfdec_test_socket_send, 0)
void
swfdec_test_socket_send (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;
  SwfdecBuffer *buffer;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "");

  if (sock->plugin == NULL) {
    swfdec_test_throw (cx, "socket is closed");
    return;
  }
  buffer = swfdec_test_buffer_from_args (cx, argc, argv);
  sock->plugin->receive (sock->plugin, buffer->data, buffer->length);
  swfdec_buffer_unref (buffer);
}

SWFDEC_TEST_FUNCTION ("Socket_receive", swfdec_test_socket_receive, 0)
void
swfdec_test_socket_receive (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;
  SwfdecBuffer *buffer;
  gsize len, depth;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "|i", &len);

  depth = swfdec_buffer_queue_get_depth (sock->receive_queue);
  if (len > 0) {
    if (depth < len) {
      swfdec_test_throw (cx, "only %zu bytes available", depth);
      return;
    } else {
      buffer = swfdec_buffer_queue_pull (sock->receive_queue, len);
    }
  } else {
    if (depth == 0) {
      SWFDEC_AS_VALUE_SET_NULL (retval);
      return;
    } else {
      buffer = swfdec_buffer_queue_pull (sock->receive_queue, depth);
    }
  }
  SWFDEC_AS_VALUE_SET_OBJECT (retval, swfdec_test_buffer_new (cx, buffer));
}

SWFDEC_TEST_FUNCTION ("Socket_error", swfdec_test_socket_error, 0)
void
swfdec_test_socket_error (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "");
  
  swfdec_test_socket_do_close (sock, FALSE);
}

SWFDEC_TEST_FUNCTION ("Socket_close", swfdec_test_socket_close_as, 0)
void
swfdec_test_socket_close_as (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "");
  
  swfdec_test_socket_do_close (sock, TRUE);
}

SWFDEC_TEST_FUNCTION ("Socket_get_closed", swfdec_test_socket_get_closed, 0)
void
swfdec_test_socket_get_closed (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "");
  
  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, sock->plugin == NULL);
}

static void 
swfdec_test_socket_plugin_send (SwfdecTestPluginSocket *plugin, unsigned char *data,
    unsigned long length)
{
  SwfdecTestSocket *sock = plugin->data;
  SwfdecBuffer *buffer;

  buffer = swfdec_buffer_new_for_data (g_memdup (data, length), length);
  swfdec_buffer_queue_push (sock->receive_queue, buffer);
}

static void
swfdec_test_socket_plugin_close (SwfdecTestPluginSocket *plugin)
{
  swfdec_test_socket_do_close (plugin->data, TRUE);
}

SwfdecAsObject *
swfdec_test_socket_new (SwfdecTestTest *test, SwfdecTestPluginSocket *plugin)
{
  SwfdecAsValue val;
  SwfdecAsContext *cx;
  SwfdecAsObject *new;

  g_return_val_if_fail (SWFDEC_IS_TEST_TEST (test), NULL);
  g_return_val_if_fail (plugin != NULL, NULL);

  cx = SWFDEC_AS_OBJECT (test)->context;

  if (!swfdec_as_context_use_mem (cx, sizeof (SwfdecTestSocket)))
    return NULL;
  new = g_object_new (SWFDEC_TYPE_TEST_SOCKET, NULL);
  swfdec_as_object_add (new, cx, sizeof (SwfdecTestSocket));
  swfdec_as_object_get_variable (cx->global, 
      swfdec_as_context_get_string (cx, "Socket"), &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val))
    swfdec_as_object_set_constructor (new, SWFDEC_AS_VALUE_GET_OBJECT (&val));

  plugin->send = swfdec_test_socket_plugin_send;
  plugin->close = swfdec_test_socket_plugin_close;
  plugin->data = new;
  SWFDEC_TEST_SOCKET (new)->test = test;
  SWFDEC_TEST_SOCKET (new)->plugin = plugin;
  test->sockets = g_list_append (test->sockets, new);

  SWFDEC_AS_VALUE_SET_STRING (&val,
      swfdec_as_context_get_string (cx, plugin->host));
  swfdec_as_object_set_variable (new, 
      swfdec_as_context_get_string (cx, "host"), &val);
  SWFDEC_AS_VALUE_SET_INT (&val, plugin->port);
  swfdec_as_object_set_variable (new, 
      swfdec_as_context_get_string (cx, "port"), &val);

  return new;
}

void
swfdec_test_socket_close (SwfdecTestSocket *sock)
{
  g_return_if_fail (SWFDEC_IS_TEST_SOCKET (sock));

  return swfdec_test_socket_do_close (sock, FALSE);
}

