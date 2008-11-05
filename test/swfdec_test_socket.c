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

G_DEFINE_TYPE (SwfdecTestSocket, swfdec_test_socket, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_test_socket_do_close (SwfdecTestSocket *sock, gboolean success)
{
  SwfdecAsContext *cx;
  SwfdecAsValue val;

  if (sock->plugin == NULL)
    return;

  cx = swfdec_gc_object_get_context (sock);
  sock->plugin->finish (sock->plugin, success ? 0 : 1);
  sock->plugin = NULL;

  sock->test->sockets = g_slist_remove (sock->test->sockets, sock);
  sock->test = NULL;

  SWFDEC_AS_VALUE_SET_BOOLEAN (&val, success);
  swfdec_as_relay_call (SWFDEC_AS_RELAY (sock), swfdec_as_context_get_string (cx, "onClose"),
      1, &val, NULL);
}

static void
swfdec_test_socket_dispose (GObject *object)
{
  SwfdecTestSocket *sock = SWFDEC_TEST_SOCKET (object);

  swfdec_test_socket_do_close (sock, FALSE);

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
}

/*** AS CODE ***/

SWFDEC_TEST_FUNCTION ("Socket_send", swfdec_test_socket_send)
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

SWFDEC_TEST_FUNCTION ("Socket_error", swfdec_test_socket_error)
void
swfdec_test_socket_error (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "");
  
  swfdec_test_socket_do_close (sock, FALSE);
}

SWFDEC_TEST_FUNCTION ("Socket_close", swfdec_test_socket_close_as)
void
swfdec_test_socket_close_as (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestSocket *sock;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_SOCKET, &sock, "");
  
  swfdec_test_socket_do_close (sock, TRUE);
}

SWFDEC_TEST_FUNCTION ("Socket_get_closed", swfdec_test_socket_get_closed)
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
  SwfdecAsContext *cx = swfdec_gc_object_get_context (sock);
  SwfdecTestBuffer *buffer;
  SwfdecAsValue val;

  buffer = swfdec_test_buffer_new (cx, swfdec_buffer_new_for_data (g_memdup (data, length), length));
  SWFDEC_AS_VALUE_SET_OBJECT (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (buffer)));
  swfdec_as_relay_call (SWFDEC_AS_RELAY (sock), swfdec_as_context_get_string (cx, "onData"),
      1, &val, NULL);
}

static void
swfdec_test_socket_plugin_close (SwfdecTestPluginSocket *plugin)
{
  swfdec_test_socket_do_close (plugin->data, TRUE);
}

SwfdecTestSocket *
swfdec_test_socket_new (SwfdecTestTest *test, SwfdecTestPluginSocket *plugin)
{
  SwfdecAsObject *object;
  SwfdecTestSocket *new;
  SwfdecAsContext *cx;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_TEST_TEST (test), NULL);
  g_return_val_if_fail (plugin != NULL, NULL);

  cx = swfdec_gc_object_get_context (test);

  new = g_object_new (SWFDEC_TYPE_TEST_SOCKET, "context", cx, NULL);
  object = swfdec_as_object_new (cx, NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (new));
  swfdec_as_object_set_constructor_by_name (object,
      swfdec_as_context_get_string (cx, "Socket"), NULL);

  plugin->send = swfdec_test_socket_plugin_send;
  plugin->close = swfdec_test_socket_plugin_close;
  plugin->data = new;
  new->test = test;
  new->plugin = plugin;
  test->sockets = g_slist_prepend (test->sockets, new);

  SWFDEC_AS_VALUE_SET_STRING (&val,
      swfdec_as_context_get_string (cx, plugin->host));
  swfdec_as_object_set_variable (object, 
      swfdec_as_context_get_string (cx, "host"), &val);
  swfdec_as_value_set_integer (cx, &val, plugin->port);
  swfdec_as_object_set_variable (object, 
      swfdec_as_context_get_string (cx, "port"), &val);

  return new;
}

void
swfdec_test_socket_close (SwfdecTestSocket *sock)
{
  g_return_if_fail (SWFDEC_IS_TEST_SOCKET (sock));

  return swfdec_test_socket_do_close (sock, FALSE);
}

