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

#include <string.h>

#include "swfdec_xml_socket.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_buffer.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"

static void
swfdec_xml_socket_ensure_closed (SwfdecXmlSocket *xml)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (xml));

  if (xml->socket == NULL)
    return;

  swfdec_stream_set_target (SWFDEC_STREAM (xml->socket), NULL);
  g_object_unref (xml->socket);
  xml->socket = NULL;
  player->priv->xml_sockets = g_slist_remove (player->priv->xml_sockets, xml);
}

/*** SWFDEC_STREAM_TARGET ***/

static SwfdecPlayer *
swfdec_xml_socket_stream_target_get_player (SwfdecStreamTarget *target)
{
  return SWFDEC_PLAYER (swfdec_gc_object_get_context (target));
}

static void
swfdec_xml_socket_stream_target_open (SwfdecStreamTarget *target, 
    SwfdecStream *stream)
{
  SwfdecXmlSocket *xml = SWFDEC_XML_SOCKET (target);
  SwfdecAsValue value;

  xml->open = TRUE;
  SWFDEC_AS_VALUE_SET_BOOLEAN (&value, TRUE);
  swfdec_sandbox_use (xml->sandbox);
  swfdec_as_object_call (xml->target, SWFDEC_AS_STR_onConnect, 1, &value, NULL);
  swfdec_sandbox_unuse (xml->sandbox);
}

static void
swfdec_xml_socket_stream_target_error (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecXmlSocket *xml = SWFDEC_XML_SOCKET (target);

  swfdec_sandbox_use (xml->sandbox);
  if (xml->open) {
    SWFDEC_FIXME ("is onClose emitted on error?");
    swfdec_as_object_call (xml->target, SWFDEC_AS_STR_onClose, 0, NULL, NULL);
  } else {
    SwfdecAsValue value;

    SWFDEC_AS_VALUE_SET_BOOLEAN (&value, FALSE);
    swfdec_as_object_call (xml->target, SWFDEC_AS_STR_onConnect, 1, &value, NULL);
  }
  swfdec_sandbox_unuse (xml->sandbox);

  swfdec_xml_socket_ensure_closed (xml);
}

static gboolean
swfdec_xml_socket_stream_target_parse (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecXmlSocket *xml = SWFDEC_XML_SOCKET (target);
  SwfdecBufferQueue *queue;
  SwfdecBuffer *buffer;
  gsize len;

  /* parse until next 0 byte or take everything */
  queue = swfdec_stream_get_queue (stream);
  while ((buffer = swfdec_buffer_queue_peek_buffer (queue))) {
    guchar *nul = memchr (buffer->data, 0, buffer->length);
    
    len = nul ? (gsize) (nul - buffer->data + 1) : buffer->length;
    g_assert (len > 0);
    swfdec_buffer_unref (buffer);
    buffer = swfdec_buffer_queue_pull (queue, len);
    swfdec_buffer_queue_push (xml->queue, buffer);
    if (nul) {
      len = swfdec_buffer_queue_get_depth (xml->queue);
      g_assert (len > 0);
      buffer = swfdec_buffer_queue_pull (xml->queue, len);
      if (!g_utf8_validate ((char *) buffer->data, len, NULL)) {
	SWFDEC_FIXME ("invalid utf8 sent through socket, what now?");
      } else {
	SwfdecAsValue val;

	SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (
	      swfdec_gc_object_get_context (xml), (char *) buffer->data));
	swfdec_sandbox_use (xml->sandbox);
	swfdec_as_object_call (xml->target, SWFDEC_AS_STR_onData, 1, &val, NULL);
	swfdec_sandbox_unuse (xml->sandbox);
      }
    }
  }
  return FALSE;
}

static void
swfdec_xml_socket_stream_target_close (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecXmlSocket *xml = SWFDEC_XML_SOCKET (target);

  if (swfdec_buffer_queue_get_depth (xml->queue)) {
    SWFDEC_FIXME ("data left in socket, what now?");
  }

  swfdec_sandbox_use (xml->sandbox);
  swfdec_as_object_call (xml->target, SWFDEC_AS_STR_onClose, 0, NULL, NULL);
  swfdec_sandbox_unuse (xml->sandbox);

  swfdec_xml_socket_ensure_closed (xml);
}

static void
swfdec_xml_socket_stream_target_init (SwfdecStreamTargetInterface *iface)
{
  iface->get_player = swfdec_xml_socket_stream_target_get_player;
  iface->open = swfdec_xml_socket_stream_target_open;
  iface->parse = swfdec_xml_socket_stream_target_parse;
  iface->close = swfdec_xml_socket_stream_target_close;
  iface->error = swfdec_xml_socket_stream_target_error;
}

/*** SWFDEC_XML_SOCKET ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecXmlSocket, swfdec_xml_socket, SWFDEC_TYPE_GC_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_STREAM_TARGET, swfdec_xml_socket_stream_target_init))

static void
swfdec_xml_socket_mark (SwfdecGcObject *object)
{
  SwfdecXmlSocket *sock = SWFDEC_XML_SOCKET (object);

  swfdec_as_object_mark (sock->target);
  swfdec_gc_object_mark (sock->sandbox);

  SWFDEC_GC_OBJECT_CLASS (swfdec_xml_socket_parent_class)->mark (object);
}

static void
swfdec_xml_socket_dispose (GObject *object)
{
  SwfdecXmlSocket *xml = SWFDEC_XML_SOCKET (object);

  swfdec_xml_socket_ensure_closed (xml);
  if (xml->queue) {
    swfdec_buffer_queue_unref (xml->queue);
    xml->queue = NULL;
  }

  G_OBJECT_CLASS (swfdec_xml_socket_parent_class)->dispose (object);
}

static void
swfdec_xml_socket_class_init (SwfdecXmlSocketClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_xml_socket_dispose;

  gc_class->mark = swfdec_xml_socket_mark;
}

static void
swfdec_xml_socket_init (SwfdecXmlSocket *xml)
{
  xml->queue = swfdec_buffer_queue_new ();
}

static SwfdecXmlSocket *
swfdec_xml_socket_create (SwfdecAsObject *target, SwfdecSandbox *sandbox, const char *hostname, guint port)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (target->context);
  SwfdecXmlSocket *xml;
  SwfdecSocket *sock;

  SWFDEC_FIXME ("implement security checks please");
  sock = swfdec_player_create_socket (player, hostname, port);
  if (sock == NULL)
    return NULL;

  xml = g_object_new (SWFDEC_TYPE_XML_SOCKET, "context", player, NULL);
  /* we prepend here, so send etc find the newest socket */
  player->priv->xml_sockets = g_slist_prepend (player->priv->xml_sockets, xml);

  xml->target = target;
  xml->socket = sock;
  xml->sandbox = sandbox;
  swfdec_stream_set_target (SWFDEC_STREAM (sock), SWFDEC_STREAM_TARGET (xml));

  return xml;
}

/*** AS CODE ***/

static SwfdecXmlSocket *
swfdec_xml_socket_get (SwfdecAsObject *object)
{
  SwfdecXmlSocket *xml;
  SwfdecPlayer *player;
  GSList *walk;

  if (object == NULL) {
    SWFDEC_WARNING ("no object to get xml socket from");
    return NULL;
  }

  player = SWFDEC_PLAYER (object->context);
  for (walk = player->priv->xml_sockets; walk; walk = walk->next) {
    xml = walk->data;

    if (xml->target == object) {
      if (xml->socket == NULL) {
	SWFDEC_WARNING ("xml socket not open");
	return NULL;
      }
      return xml;
    }
  }

  SWFDEC_WARNING ("no xml socket on object");
  return NULL;
}


SWFDEC_AS_NATIVE (400, 0, swfdec_xml_socket_connect)
void
swfdec_xml_socket_connect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *host;
  int port;

  SWFDEC_AS_CHECK (0, NULL, "si", &host, &port);

  if (object == NULL || object->movie)
    return;

  swfdec_xml_socket_create (object, swfdec_sandbox_get (SWFDEC_PLAYER (cx)), host, port);
}

SWFDEC_AS_NATIVE (400, 1, swfdec_xml_socket_send)
void
swfdec_xml_socket_send (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlSocket *xml;
  SwfdecBuffer *buf;
  const char *send;
  gsize len;

  SWFDEC_AS_CHECK (0, NULL, "s", &send);
  if (argc < 1)
    return;

  xml = swfdec_xml_socket_get (object);
  if (xml == NULL)
    return;
  if (!swfdec_stream_is_open (SWFDEC_STREAM (xml->socket))) {
    SWFDEC_WARNING ("sending data down a closed stream");
    return;
  }

  len = strlen (send) + 1;
  buf = swfdec_buffer_new (len);
  memcpy (buf->data, send, len);

  swfdec_socket_send (xml->socket, buf);
}

SWFDEC_AS_NATIVE (400, 2, swfdec_xml_socket_close)
void
swfdec_xml_socket_close (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlSocket *xml;

  xml = swfdec_xml_socket_get (object);
  if (xml == NULL)
    return;
  swfdec_stream_ensure_closed (SWFDEC_STREAM (xml->socket));
}
