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
    playing = !swfdec_as_value_to_boolean (cx, &argv[0]);
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

  cur = SWFDEC_SANDBOX (cx->global);
  swfdec_sandbox_unuse (cur);
  /* FIXME: perform security check if seeking is allowed here? */
  swfdec_net_stream_seek (stream, d);
  swfdec_sandbox_use (cur);
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
  stream = SWFDEC_NET_STREAM (obj);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]) || 
      !SWFDEC_IS_NET_CONNECTION ((conn = (SwfdecNetConnection *) SWFDEC_AS_VALUE_GET_OBJECT (&argv[0])))) {
    SWFDEC_WARNING ("no connection passed to NetStream ()");
    return;
  }
  stream->conn = conn;
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
  stream = SWFDEC_AS_OBJECT (swfdec_as_object_add_constructor (context->global, 
      SWFDEC_AS_STR_NetStream, SWFDEC_TYPE_NET_STREAM, SWFDEC_TYPE_NET_STREAM,
      swfdec_net_stream_construct, 1, proto));
  /* set the right properties on the NetStream.prototype object */
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_pause, 0,
      swfdec_net_stream_pause, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_play, 0,
      swfdec_net_stream_play, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_seek, 0,
      swfdec_net_stream_do_seek, 0);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, stream);
  swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR_constructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR___proto__, &val,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
}

