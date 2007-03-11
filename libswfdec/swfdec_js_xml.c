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

#include "swfdec_xml.h"
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_player_internal.h"

static JSBool
swfdec_js_xml_load (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecXml *xml;
  const char *url;

  xml = JS_GetPrivate (cx, obj);
  if (xml == NULL)
    return JS_TRUE;
  url = swfdec_js_to_string (cx, argv[0]);
  if (url == NULL)
    return JS_FALSE;
  swfdec_xml_load (xml, url);
  return JS_TRUE;
}

static JSFunctionSpec xml_methods[] = {
  { "load",		swfdec_js_xml_load,		1, 0, 0 },
  {0,0,0,0,0}
};

static void
swfdec_js_xml_finalize (JSContext *cx, JSObject *obj)
{
  SwfdecXml *conn;

  conn = JS_GetPrivate (cx, obj);
  if (conn) {
    SWFDEC_SCRIPTABLE (conn)->jsobj = NULL;
    g_object_unref (conn);
  }
}

const JSClass xml_class = {
    "XML", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   swfdec_js_xml_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
swfdec_js_xml_new (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecXml *xml = swfdec_xml_new (JS_GetContextPrivate (cx));

  JS_SetPrivate (cx, obj, xml);
  SWFDEC_SCRIPTABLE (xml)->jsobj = obj;
  *rval = OBJECT_TO_JSVAL (obj);
  return JS_TRUE;
}

void
swfdec_js_add_xml (SwfdecPlayer *player)
{
  JS_InitClass (player->jscx, player->jsobj, NULL,
      &xml_class, swfdec_js_xml_new, 0, NULL, xml_methods,
      NULL, NULL);
}

