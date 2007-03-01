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
swfdec_js_net_stream_to_string (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  JSString *string;

  string = JS_InternString (cx, "[object Object]");
  if (string == NULL)
    return JS_FALSE;

  *rval = STRING_TO_JSVAL (string);
  return JS_TRUE;
}

static JSFunctionSpec net_stream_methods[] = {
  { "toString",		swfdec_js_net_stream_to_string,	0, 0, 0 },
  {0,0,0,0,0}
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
      &net_stream_class, swfdec_js_net_stream_new, 0, NULL, net_stream_methods,
      NULL, NULL);
}

