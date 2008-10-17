/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include <libsoup/soup.h>

#include "swfdec_test_http_server.h"
#include "swfdec_test_http_request.h"
#include "swfdec_test_function.h"
#include "swfdec_test_utils.h"

static void
swfdec_test_http_server_callback (SoupServer *soup_server,
    SoupMessage *message, const char *path, GHashTable *query,
    SoupClientContext *client, gpointer userdata)
{
  SwfdecTestHTTPServer *server = userdata;

  g_return_if_fail (server->server == soup_server);

  soup_server_pause_message (server->server, message);

  g_queue_push_head (server->messages, g_object_ref (message));
}

void
swfdec_test_http_server_run (SwfdecTestHTTPServer *server)
{
  while (g_main_context_iteration (server->context, FALSE));
}

static gboolean
swfdec_test_http_server_start (SwfdecTestHTTPServer *server)
{
  SoupAddress *address;

  g_return_val_if_fail (SWFDEC_IS_TEST_HTTP_SERVER (server), FALSE);
  g_return_val_if_fail (server->server == NULL, FALSE);

  address = soup_address_new ("localhost", server->port);
  if (soup_address_resolve_sync (address, NULL) != SOUP_STATUS_OK)
    return FALSE;

  server->context = g_main_context_new ();

  server->server = soup_server_new (SOUP_SERVER_PORT, server->port,
      SOUP_SERVER_INTERFACE, address, SOUP_SERVER_ASYNC_CONTEXT,
      server->context, NULL);
  if (!server->server) {
    g_main_context_unref (server->context);
    server->context = NULL;
    return FALSE;
  }

  soup_server_add_handler (server->server, NULL,
      swfdec_test_http_server_callback, server, NULL);

  soup_server_run_async (server->server);

  return TRUE;
}

SwfdecAsObject *
swfdec_test_http_server_new (SwfdecAsContext *context, guint port)
{
  SwfdecAsValue val;
  SwfdecAsObject *ret;

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecTestHTTPServer)))
    return NULL;

  ret = g_object_new (SWFDEC_TYPE_TEST_HTTP_SERVER, NULL);
  swfdec_as_object_add (ret, context, sizeof (SwfdecTestHTTPServer));
  swfdec_as_object_get_variable (context->global,
      swfdec_as_context_get_string (context, "HTTPServer"), &val);

  if (SWFDEC_AS_VALUE_IS_OBJECT (&val))
    swfdec_as_object_set_constructor (ret, SWFDEC_AS_VALUE_GET_OBJECT (&val));

  SWFDEC_TEST_HTTP_SERVER (ret)->port = port;

  if (!swfdec_test_http_server_start (SWFDEC_TEST_HTTP_SERVER (ret))) {
    swfdec_test_throw (context, "Failed to start web server for port %i",
	port);
    return NULL;
  }

  return ret;
}

/*** SWFDEC_TEST_HTTP_SERVER ***/

G_DEFINE_TYPE (SwfdecTestHTTPServer, swfdec_test_http_server, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_test_http_server_dispose (GObject *object)
{
  SwfdecTestHTTPServer *server = SWFDEC_TEST_HTTP_SERVER (object);

  while (!g_queue_is_empty (server->messages)) {
    g_object_unref (g_queue_pop_tail (server->messages));
  }
  g_queue_free (server->messages);
  server->messages = NULL;

  if (server->server) {
    soup_server_quit (server->server);
    g_object_unref (server->server);
    server->server = NULL;
  }

  if (server->context) {
    g_main_context_unref (server->context);
    server->context = NULL;
  }

  G_OBJECT_CLASS (swfdec_test_http_server_parent_class)->dispose (object);
}

static void
swfdec_test_http_server_class_init (SwfdecTestHTTPServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_test_http_server_dispose;
}

static void
swfdec_test_http_server_init (SwfdecTestHTTPServer *server)
{
  server->messages = g_queue_new ();
}

/*** AS CODE ***/

SWFDEC_TEST_FUNCTION ("HTTPServer_get_port", swfdec_test_http_server_get_port, 0)
void
swfdec_test_http_server_get_port (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestHTTPServer *server;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &server, "");

  swfdec_as_value_set_integer (cx, retval, server->port);
}

SWFDEC_TEST_FUNCTION ("HTTPServer_getRequest", swfdec_test_http_server_get_request, 0)
void
swfdec_test_http_server_get_request (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *retval)
{
  SwfdecTestHTTPServer *server;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_SERVER, &server, "");

  if (!server->server) {
    swfdec_test_throw (cx, "Webserver not running");
    return;
  }

  if (g_queue_is_empty (server->messages))
    swfdec_test_http_server_run (server);

  if (g_queue_is_empty (server->messages)) {
    SWFDEC_AS_VALUE_SET_NULL (retval);
  } else {
    SwfdecAsObject *request;

    request = swfdec_test_http_request_new (cx, server,
	g_queue_pop_tail (server->messages));

    SWFDEC_AS_VALUE_SET_OBJECT (retval, request);
  }
}

SWFDEC_TEST_FUNCTION ("HTTPServer", swfdec_test_http_server_create, swfdec_test_http_server_get_type)
void
swfdec_test_http_server_create (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestHTTPServer *server;
  int port;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_SERVER, &server, "i", &port);

  if (port <= 0)
    return;

  server->port = port;

  if (!swfdec_test_http_server_start (server))
    swfdec_test_throw (cx, "Failed to start web server for port %i", port);
}
