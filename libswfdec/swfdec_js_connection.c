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

#include "swfdec_connection.h"
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_player_internal.h"

static JSBool
swfdec_js_connection_connect (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  const char *url;
  SwfdecConnection *conn = SWFDEC_CONNECTION (JS_GetPrivate (cx, obj));

  if (conn == NULL) {
    SWFDEC_INFO ("connect called on prototype, ignoring");
    return JS_TRUE;
  }
  if (JSVAL_IS_STRING (argv[0])) {
    url = swfdec_js_to_string (cx, argv[0]);
    if (url == NULL)
      return JS_FALSE;
  } else if (JSVAL_IS_NULL (argv[0])) {
    url = NULL;
  } else {
    SWFDEC_INFO ("untested argument to NetConnection.connect: %lu", argv[0]);
    url = NULL;
  }
  swfdec_connection_connect (conn, url);
  return JS_TRUE;
}

static JSBool
swfdec_js_connection_to_string (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  JSString *string;

  string = JS_InternString (cx, "[object Object]");
  if (string == NULL)
    return JS_FALSE;

  *rval = STRING_TO_JSVAL (string);
  return JS_TRUE;
}

static JSFunctionSpec connection_methods[] = {
  { "connect",		swfdec_js_connection_connect,	1, 0, 0 },
  { "toString",		swfdec_js_connection_to_string,	0, 0, 0 },
  {0,0,0,0,0}
};

static void
swfdec_js_connection_finalize (JSContext *cx, JSObject *obj)
{
  SwfdecConnection *conn;

  conn = JS_GetPrivate (cx, obj);
  if (conn) {
    SWFDEC_SCRIPTABLE (conn)->jsobj = NULL;
    g_object_unref (conn);
  }
}

const JSClass connection_class = {
    "NetConnection", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   swfdec_js_connection_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
swfdec_js_connection_new (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecConnection *conn = swfdec_connection_new (cx);

  JS_SetPrivate (cx, obj, conn);
  SWFDEC_SCRIPTABLE (conn)->jsobj = obj;
  *rval = OBJECT_TO_JSVAL (obj);
  return JS_TRUE;
}

void
swfdec_js_add_connection (SwfdecPlayer *player)
{
  JS_InitClass (player->jscx, player->jsobj, NULL,
      &connection_class, swfdec_js_connection_new, 0, NULL, connection_methods,
      NULL, NULL);
}

