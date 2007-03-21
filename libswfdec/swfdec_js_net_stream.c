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
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_player_internal.h"

static JSBool
swfdec_js_net_stream_play (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecNetStream *stream;
  const char *url;

  stream = swfdec_scriptable_from_object (cx, obj, SWFDEC_TYPE_NET_STREAM);
  if (stream == NULL)
    return JS_TRUE;
  url = swfdec_js_to_string (cx, argv[0]);
  if (url == NULL)
    return JS_FALSE;
  swfdec_net_stream_set_url (stream, url);
  swfdec_net_stream_set_playing (stream, TRUE);
  return JS_TRUE;
}

static JSBool
swfdec_js_net_stream_pause (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecNetStream *stream;
  gboolean playing;

  stream = swfdec_scriptable_from_object (cx, obj, SWFDEC_TYPE_NET_STREAM);
  if (stream == NULL)
    return JS_TRUE;
  if (argc == 0) {
    playing = !swfdec_net_stream_get_playing (stream);
  } else {
    JSBool b;
    if (!JS_ValueToBoolean (cx, argv[0], &b))
      return JS_FALSE;
    playing = !b;
  }
  SWFDEC_LOG ("%s stream %p", playing ? "playing" : "pausing", stream);
  swfdec_net_stream_set_playing (stream, playing);
  return JS_TRUE;
}

static JSBool
swfdec_js_net_stream_set_buffer_time (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecNetStream *stream;
  double d;

  stream = swfdec_scriptable_from_object (cx, obj, SWFDEC_TYPE_NET_STREAM);
  if (stream == NULL)
    return JS_TRUE;
  if (!JS_ValueToNumber (cx, argv[0], &d))
    return JS_FALSE;
  swfdec_net_stream_set_buffer_time (stream, d);
  return JS_TRUE;
}

static JSFunctionSpec net_stream_methods[] = {
  { "pause",		swfdec_js_net_stream_pause,		0, 0, 0 },
  { "play",		swfdec_js_net_stream_play,		1, 0, 0 },
  { "setBufferTime",  	swfdec_js_net_stream_set_buffer_time,	1, 0, 0 },
  { NULL }
};

static JSBool
swfdec_js_net_stream_time (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecNetStream *stream;
  guint msecs;

  stream = swfdec_scriptable_from_object (cx, obj, SWFDEC_TYPE_NET_STREAM);
  if (stream == NULL)
    return JS_TRUE;

  if (stream->flvdecoder == NULL ||
      !swfdec_flv_decoder_get_video_info (stream->flvdecoder, &msecs, NULL)) {
    *vp = INT_TO_JSVAL (0);
    return JS_TRUE;
  }
  if (msecs >= stream->current_time)
    msecs = 0;
  else
    msecs = stream->current_time - msecs;

  return JS_NewNumberValue (cx, msecs / 1000., vp);
}

static JSBool
swfdec_js_net_stream_bytes_loaded (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecNetStream *stream;

  stream = swfdec_scriptable_from_object (cx, obj, SWFDEC_TYPE_NET_STREAM);
  if (stream == NULL)
    return JS_TRUE;

  if (stream->loader == NULL) {
    *vp = INT_TO_JSVAL (0);
    return JS_TRUE;
  }

  return JS_NewNumberValue (cx, swfdec_loader_get_loaded (stream->loader), vp);
}

static JSBool
swfdec_js_net_stream_bytes_total (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecNetStream *stream;
  gulong bytes;

  stream = swfdec_scriptable_from_object (cx, obj, SWFDEC_TYPE_NET_STREAM);
  if (stream == NULL)
    return JS_TRUE;

  if (stream->loader == NULL) {
    *vp = INT_TO_JSVAL (0);
    return JS_TRUE;
  }
  bytes = swfdec_loader_get_size (stream->loader);
  if (bytes == 0)
    bytes = swfdec_loader_get_loaded (stream->loader);

  return JS_NewNumberValue (cx, bytes, vp);
}

static JSPropertySpec net_stream_props[] = {
  { "bytesLoaded",	-1,	JSPROP_PERMANENT|JSPROP_READONLY,	swfdec_js_net_stream_bytes_loaded,	NULL },
  { "bytesTotal",	-1,	JSPROP_PERMANENT|JSPROP_READONLY,	swfdec_js_net_stream_bytes_total,	NULL },
  { "time",		-1,	JSPROP_PERMANENT|JSPROP_READONLY,	swfdec_js_net_stream_time,		NULL },
  { NULL }
};

static void
swfdec_js_net_stream_finalize (JSContext *cx, JSObject *obj)
{
  SwfdecNetStream *stream;

  stream = JS_GetPrivate (cx, obj);
  if (stream) {
    SWFDEC_SCRIPTABLE (stream)->jsobj = NULL;
    g_object_unref (stream);
  }
}

const JSClass net_stream_class = {
    "NetStream", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   swfdec_js_net_stream_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
swfdec_js_net_stream_new (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecNetStream *stream;
  SwfdecConnection *conn;
  
  conn = swfdec_scriptable_from_jsval (cx, argv[0], SWFDEC_TYPE_CONNECTION);
  if (conn == NULL) {
    SWFDEC_WARNING ("no connection passed to NetStream ()");
    *rval = JSVAL_VOID;
    return JS_TRUE;
  }
  stream = swfdec_net_stream_new (JS_GetContextPrivate (cx), conn);

  JS_SetPrivate (cx, obj, stream);
  SWFDEC_SCRIPTABLE (stream)->jsobj = obj;

  *rval = OBJECT_TO_JSVAL (obj);
  return JS_TRUE;
}

void
swfdec_js_add_net_stream (SwfdecPlayer *player)
{
  JS_InitClass (player->jscx, player->jsobj, NULL,
      &net_stream_class, swfdec_js_net_stream_new, 0, net_stream_props, net_stream_methods,
      NULL, NULL);
}

