/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_net_stream.h"
#include "swfdec_as_context.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_sandbox.h"

SWFDEC_AS_NATIVE (2101, 0, swfdec_net_stream_close)
void
swfdec_net_stream_close (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecNetStream *stream;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_NET_STREAM, &stream, "");

  swfdec_net_stream_set_loader (stream, NULL);
  swfdec_net_stream_set_playing (stream, TRUE);
}

static void
swfdec_net_stream_play (SwfdecAsContext *cx, SwfdecAsObject *object, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecNetStream *stream;
  const char *url;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_NET_STREAM, &stream, "s", &url);

  swfdec_net_stream_set_url (stream, url);
  swfdec_net_stream_set_playing (stream, TRUE);
}

static void
swfdec_net_stream_pause (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecNetStream *stream;
  gboolean playing;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_NET_STREAM, &stream, "");

  if (argc == 0) {
    playing = !swfdec_net_stream_get_playing (stream);
  } else {
    playing = !swfdec_as_value_to_boolean (cx, argv[0]);
  }
  SWFDEC_LOG ("%s stream %p", playing ? "playing" : "pausing", stream);
  swfdec_net_stream_set_playing (stream, playing);
}

SWFDEC_AS_NATIVE (2101, 1, swfdec_net_stream_attachAudio)
void
swfdec_net_stream_attachAudio (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetStream.attachAudio");
}

SWFDEC_AS_NATIVE (2101, 2, swfdec_net_stream_attachVideo)
void
swfdec_net_stream_attachVideo (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetStream.attachVideo");
}

SWFDEC_AS_NATIVE (2101, 3, swfdec_net_stream_send)
void
swfdec_net_stream_send (SwfdecAsContext *cx, SwfdecAsObject *obj, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetStream.send");
}

SWFDEC_AS_NATIVE (2101, 4, swfdec_net_stream_setBufferTime)
void
swfdec_net_stream_setBufferTime (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecNetStream *stream;
  double d;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_NET_STREAM, &stream, "n", &d);

  swfdec_net_stream_set_buffer_time (stream, d);
}

SWFDEC_AS_NATIVE (2101, 5, swfdec_net_stream_get_checkPolicyFile)
void
swfdec_net_stream_get_checkPolicyFile (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetStream.checkPolicyFile (get)");
}

SWFDEC_AS_NATIVE (2101, 6, swfdec_net_stream_set_checkPolicyFile)
void
swfdec_net_stream_set_checkPolicyFile (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetStream.checkPolicyFile (set)");
}

static void
swfdec_net_stream_do_seek (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecNetStream *stream;
  SwfdecSandbox *cur;
  double d;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_NET_STREAM, &stream, "n", &d);

  cur = swfdec_sandbox_get (SWFDEC_PLAYER (cx));
  swfdec_sandbox_unuse (cur);
  /* FIXME: perform security check if seeking is allowed here? */
  swfdec_net_stream_seek (stream, d);
  swfdec_sandbox_use (cur);
}

static void
swfdec_net_stream_get_time (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecNetStream *stream;
  guint msecs;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_NET_STREAM, &stream, "");

  if (stream->flvdecoder == NULL ||
      !swfdec_flv_decoder_get_video_info (stream->flvdecoder, &msecs, NULL)) {
    *ret = swfdec_as_value_from_integer (cx, 0);
  } else {
    if (msecs >= stream->current_time)
      msecs = 0;
    else 
      msecs = stream->current_time - msecs;
    *ret = swfdec_as_value_from_number (cx, msecs / 1000.);
  }
}

static void
swfdec_net_stream_get_bytesLoaded (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecNetStream *stream;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_NET_STREAM, &stream, "");

  if (stream->loader == NULL)
    *ret = swfdec_as_value_from_integer (cx, 0);
  else
    *ret = swfdec_as_value_from_number (cx, swfdec_loader_get_loaded (stream->loader));
}

static void
swfdec_net_stream_get_bytesTotal (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecNetStream *stream;
  glong bytes;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_NET_STREAM, &stream, "");

  if (stream->loader == NULL) {
    bytes = 0;
  } else { 
    bytes = swfdec_loader_get_size (stream->loader);
    if (bytes < 0)
      bytes = swfdec_loader_get_loaded (stream->loader);
  }
  *ret = swfdec_as_value_from_number (cx, bytes);
}

static void
swfdec_net_stream_get_bufferLength (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Netstream.bufferLength (get)");
}

static void
swfdec_net_stream_get_bufferTime (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Netstream.bufferTime (get)");
}

static void
swfdec_net_stream_get_audiocodec (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Netstream.audiocodec (get)");
}

static void
swfdec_net_stream_get_currentFps (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Netstream.currentFps (get)");
}

static void
swfdec_net_stream_get_decodedFrames (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Netstream.decodedFrames (get)");
}

static void
swfdec_net_stream_get_liveDelay (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Netstream.liveDelay (get)");
}

static void
swfdec_net_stream_get_videoCodec (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Netstream.videoCodec (get)");
}

SWFDEC_AS_NATIVE (2101, 200, swfdec_net_stream_setup)
void
swfdec_net_stream_setup (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (object == NULL)
    return;

  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_time,
      swfdec_net_stream_get_time, NULL);
  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_bytesLoaded,
      swfdec_net_stream_get_bytesLoaded, NULL);
  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_bytesTotal,
      swfdec_net_stream_get_bytesTotal, NULL);
  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_bufferLength,
      swfdec_net_stream_get_bufferLength, NULL);
  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_bufferTime,
      swfdec_net_stream_get_bufferTime, NULL);
  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_audiocodec,
      swfdec_net_stream_get_audiocodec, NULL);
  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_currentFps,
      swfdec_net_stream_get_currentFps, NULL);
  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_decodedFrames,
      swfdec_net_stream_get_decodedFrames, NULL);
  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_liveDelay,
      swfdec_net_stream_get_liveDelay, NULL);
  swfdec_as_object_add_native_variable (object, SWFDEC_AS_STR_videoCodec,
      swfdec_net_stream_get_videoCodec, NULL);
}

static void
swfdec_net_stream_construct (SwfdecAsContext *cx, SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecNetStream *stream;
  SwfdecNetConnection *conn;
  
  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }
  stream = g_object_new (SWFDEC_TYPE_NET_STREAM, "context", cx, NULL);
  swfdec_as_object_set_relay (obj, SWFDEC_AS_RELAY (stream));

  swfdec_net_stream_setup (cx, obj, 0, NULL, rval);
  if (argc == 0 ||
      !SWFDEC_AS_VALUE_IS_OBJECT (argv[0]) || 
      !SWFDEC_IS_NET_CONNECTION ((conn = (SwfdecNetConnection *) SWFDEC_AS_VALUE_GET_OBJECT (argv[0])))) {
    SWFDEC_WARNING ("no connection passed to NetStream ()");
    return;
  }
  stream->conn = conn;
  SWFDEC_AS_VALUE_SET_OBJECT (rval, obj);
}

void
swfdec_net_stream_init_context (SwfdecPlayer *player)
{
  SwfdecAsContext *context;
  SwfdecAsObject *stream, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  context = SWFDEC_AS_CONTEXT (player);
  proto = swfdec_as_object_new_empty (context);
  stream = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (
	swfdec_as_object_add_function (context->global, 
	  SWFDEC_AS_STR_NetStream, swfdec_net_stream_construct)));
  /* set the right properties on the NetStream.prototype object */
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_pause, swfdec_net_stream_pause);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_play, swfdec_net_stream_play);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_seek, swfdec_net_stream_do_seek);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, stream);
  swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_Object, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    swfdec_as_object_get_variable (SWFDEC_AS_VALUE_GET_OBJECT (val),
	SWFDEC_AS_STR_prototype, &val);
    swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR___proto__, &val,
	SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
    SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
    swfdec_as_object_set_variable_and_flags (stream, SWFDEC_AS_STR_prototype, &val,
	SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  }
}

