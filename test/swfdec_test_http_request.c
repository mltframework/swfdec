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

#include <string.h>

#include "swfdec_test_http_request.h"
#include "swfdec_test_http_server.h"
#include "swfdec_test_function.h"

SwfdecAsObject *
swfdec_test_http_request_new (SwfdecAsContext *context,
    SwfdecTestHTTPServer *server, SoupMessage *message)
{
  SwfdecAsValue val;
  SwfdecAsObject *ret;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (SWFDEC_IS_TEST_HTTP_SERVER (server), NULL);
  g_return_val_if_fail (SOUP_IS_MESSAGE (message), NULL);

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecTestHTTPRequest)))
    return NULL;

  ret = g_object_new (SWFDEC_TYPE_TEST_HTTP_REQUEST, NULL);
  swfdec_as_object_add (ret, context, sizeof (SwfdecTestHTTPRequest));
  swfdec_as_object_get_variable (context->global,
      swfdec_as_context_get_string (context, "HTTPRequest"), &val);

  if (SWFDEC_AS_VALUE_IS_OBJECT (&val))
    swfdec_as_object_set_constructor (ret, SWFDEC_AS_VALUE_GET_OBJECT (&val));

  SWFDEC_TEST_HTTP_REQUEST (ret)->server = server;
  SWFDEC_TEST_HTTP_REQUEST (ret)->message = message;
  SWFDEC_TEST_HTTP_REQUEST (ret)->state =
    SWFDEC_TEST_HTTP_REQUEST_STATE_WAITING;

  return ret;
}

/*** SWFDEC_TEST_HTTP_REQUEST ***/

G_DEFINE_TYPE (SwfdecTestHTTPRequest, swfdec_test_http_request, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_test_http_request_dispose (GObject *object)
{
  SwfdecTestHTTPRequest *request = SWFDEC_TEST_HTTP_REQUEST (object);

  g_object_unref (request->message);
  request->message = NULL;

  G_OBJECT_CLASS (swfdec_test_http_request_parent_class)->dispose (object);
}

static void
swfdec_test_http_request_mark (SwfdecAsObject *object)
{
  SwfdecTestHTTPRequest *request = SWFDEC_TEST_HTTP_REQUEST (object);

  swfdec_as_object_mark (SWFDEC_AS_OBJECT (request->server));

  SWFDEC_AS_OBJECT_CLASS (swfdec_test_http_request_parent_class)->mark (object);
}

static void
swfdec_test_http_request_class_init (SwfdecTestHTTPRequestClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_test_http_request_dispose;
  asobject_class->mark = swfdec_test_http_request_mark;
}

static void
swfdec_test_http_request_init (SwfdecTestHTTPRequest *this)
{
}

/*** AS CODE ***/

SWFDEC_TEST_FUNCTION ("HTTPRequest_toString", swfdec_test_http_request_toString, 0)
void
swfdec_test_http_request_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;
  GString *string;
  SoupURI *uri;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "");

  string = g_string_new (request->message->method);
  string = g_string_append (string, " ");

  uri = soup_message_get_uri (request->message);
  string = g_string_append (string, soup_uri_to_string (uri, FALSE));

  SWFDEC_AS_VALUE_SET_STRING (retval,
      swfdec_as_context_give_string (cx, g_string_free (string, FALSE)));

  soup_uri_free (uri);
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_get_server", swfdec_test_http_request_get_server, 0)
void
swfdec_test_http_request_get_server (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "");

  SWFDEC_AS_VALUE_SET_OBJECT (retval, SWFDEC_AS_OBJECT (request->server));
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_get_url", swfdec_test_http_request_get_url, 0)
void
swfdec_test_http_request_get_url (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;
  SoupURI *uri;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "");

  uri = soup_message_get_uri (request->message);

  SWFDEC_AS_VALUE_SET_STRING (retval,
      swfdec_as_context_give_string (cx, soup_uri_to_string (uri, FALSE)));

  soup_uri_free (uri);
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_get_path", swfdec_test_http_request_get_path, 0)
void
swfdec_test_http_request_get_path (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;
  SoupURI *uri;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "");

  uri = soup_message_get_uri (request->message);

  SWFDEC_AS_VALUE_SET_STRING (retval,
      swfdec_as_context_give_string (cx, soup_uri_to_string (uri, TRUE)));

  soup_uri_free (uri);
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_push", swfdec_test_http_request_push, 0)
void
swfdec_test_http_request_push (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;
  const char *data;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "s", &data);

  if (request->state > SWFDEC_TEST_HTTP_REQUEST_STATE_SENDING)
    return;

  if (!request->status_set) {
    soup_message_set_status (request->message, SOUP_STATUS_OK);
    request->status_set = TRUE;
  }

  soup_message_body_append (request->message->response_body, SOUP_MEMORY_COPY,
      data, strlen (data));
  soup_server_unpause_message (request->server->server, request->message);

  swfdec_test_http_server_run (request->server);

  request->state = SWFDEC_TEST_HTTP_REQUEST_STATE_SENDING;
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_close", swfdec_test_http_request_close, 0)
void
swfdec_test_http_request_close (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "");

  if (request->state == SWFDEC_TEST_HTTP_REQUEST_STATE_SENT)
    return;

  if (!request->status_set) {
    soup_message_set_status (request->message, SOUP_STATUS_OK);
    request->status_set = TRUE;
  }

  soup_message_body_complete (request->message->response_body);
  soup_server_unpause_message (request->server->server, request->message);

  swfdec_test_http_server_run (request->server);

  request->state = SWFDEC_TEST_HTTP_REQUEST_STATE_SENT;
}
