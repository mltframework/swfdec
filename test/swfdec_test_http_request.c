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
#include "swfdec_test_buffer.h"
#include "swfdec_test_function.h"
#include "swfdec_test_utils.h"

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

  soup_message_headers_set_encoding (message->response_headers,
      SOUP_ENCODING_CHUNKED);
  soup_message_set_flags (message, SOUP_MESSAGE_OVERWRITE_CHUNKS);
  soup_message_set_status (message, SOUP_STATUS_OK);

  SWFDEC_TEST_HTTP_REQUEST (ret)->server = server;
  SWFDEC_TEST_HTTP_REQUEST (ret)->message = message;

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
  swfdec_as_object_mark (SWFDEC_AS_OBJECT (request->headers));

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
}

static void
swfdec_test_http_request_foreach_set_headers (const char *name,
    const char *value, gpointer user_data)
{
  SwfdecTestHTTPRequest *request = user_data;
  SwfdecAsContext *cx;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (request->headers));

  cx = request->headers->context;

  SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (cx, value));
  swfdec_as_object_set_variable (request->headers,
      swfdec_as_context_get_string (cx, name), &val);
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_get_headers", swfdec_test_http_request_get_headers, 0)
void
swfdec_test_http_request_get_headers (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "");

  if (!request->headers) {
    request->headers = swfdec_as_object_new_empty (cx);
    soup_message_headers_foreach (request->message->request_headers,
	swfdec_test_http_request_foreach_set_headers, request);
  }

  SWFDEC_AS_VALUE_SET_OBJECT (retval, request->headers);
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_get_contentType", swfdec_test_http_request_get_contentType, 0)
void
swfdec_test_http_request_get_contentType (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;
  const char *value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "");

  value = soup_message_headers_get (request->message->response_headers,
      "Content-Type");

  if (value != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (retval,
	swfdec_as_context_get_string (cx, value));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (retval);
  }
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_set_contentType", swfdec_test_http_request_set_contentType, 0)
void
swfdec_test_http_request_set_contentType (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "v", &val);

  if (request->state > SWFDEC_TEST_HTTP_REQUEST_STATE_WAITING) {
    swfdec_test_throw (cx, "Headers have already been sent");
    return;
  }

  if (SWFDEC_AS_VALUE_IS_NULL (&val) || SWFDEC_AS_VALUE_IS_UNDEFINED (&val)) {
    soup_message_headers_remove (request->message->response_headers,
	"Content-Type");
  } else {
    soup_message_headers_replace (request->message->response_headers,
	"Content-Type", swfdec_as_value_to_string (cx, &val));
  }
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_get_statusCode", swfdec_test_http_request_get_statusCode, 0)
void
swfdec_test_http_request_get_statusCode (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "");

  SWFDEC_AS_VALUE_SET_INT (retval, request->message->status_code);
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_set_statusCode", swfdec_test_http_request_set_statusCode, 0)
void
swfdec_test_http_request_set_statusCode (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;
  int status_code;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "i", &status_code);

  if (status_code < 0)
    return;

  if (request->state > SWFDEC_TEST_HTTP_REQUEST_STATE_WAITING) {
    swfdec_test_throw (cx, "Headers have already been sent");
    return;
  }

  soup_message_set_status (request->message, status_code);
}

SWFDEC_TEST_FUNCTION ("HTTPRequest_send", swfdec_test_http_request_send, 0)
void
swfdec_test_http_request_send (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestHTTPRequest *request;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_HTTP_REQUEST, &request, "v", &val);

  if (request->state > SWFDEC_TEST_HTTP_REQUEST_STATE_SENDING) {
    swfdec_test_throw (cx, "Reply has already been sent");
    return;
  }

  if (SWFDEC_AS_VALUE_IS_OBJECT (&val) &&
      SWFDEC_IS_TEST_BUFFER (SWFDEC_AS_VALUE_GET_OBJECT (&val))) {
    SwfdecTestBuffer *buffer =
      SWFDEC_TEST_BUFFER (SWFDEC_AS_VALUE_GET_OBJECT (&val));
    soup_message_body_append (request->message->response_body,
	SOUP_MEMORY_COPY, buffer->buffer->data, buffer->buffer->length);
  } else {
    const char *data = swfdec_as_value_to_string (cx, &val);
    soup_message_body_append (request->message->response_body,
	SOUP_MEMORY_COPY, data, strlen (data));
  }

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

  if (request->state == SWFDEC_TEST_HTTP_REQUEST_STATE_SENT) {
    swfdec_test_throw (cx, "Reply has already been sent");
    return;
  }

  soup_message_body_complete (request->message->response_body);
  soup_server_unpause_message (request->server->server, request->message);

  swfdec_test_http_server_run (request->server);

  request->state = SWFDEC_TEST_HTTP_REQUEST_STATE_SENT;
}
