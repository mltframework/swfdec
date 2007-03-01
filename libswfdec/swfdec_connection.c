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

#include <string.h>
#include "swfdec_connection.h"
#include "swfdec_debug.h"
#include "js/jsapi.h"
#include "js/jsinterp.h"

/*** SwfdecConnection ***/

G_DEFINE_TYPE (SwfdecConnection, swfdec_connection, SWFDEC_TYPE_SCRIPTABLE)

static void
swfdec_connection_dispose (GObject *object)
{
  SwfdecConnection *connection = SWFDEC_CONNECTION (object);

  g_free (connection->url);
  connection->url = NULL;

  G_OBJECT_CLASS (swfdec_connection_parent_class)->dispose (object);
}

extern const JSClass connection_class;
static void
swfdec_connection_class_init (SwfdecConnectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecScriptableClass *scriptable_class = SWFDEC_SCRIPTABLE_CLASS (klass);

  object_class->dispose = swfdec_connection_dispose;

  scriptable_class->jsclass = &connection_class;
}

static void
swfdec_connection_init (SwfdecConnection *connection)
{
}

static void
swfdec_connection_onstatus (SwfdecConnection *conn, const char *code,
    const char *level, const char *description)
{
  JSContext *cx = SWFDEC_SCRIPTABLE (conn)->jscx;
  JSObject *obj = SWFDEC_SCRIPTABLE (conn)->jsobj;
  JSObject *info;
  jsval val, fun;
  JSString *string;

  if (!JS_GetProperty (cx, obj, "onStatus", &fun))
    return;
  if (fun == JSVAL_VOID)
    return;
  info = JS_NewObject (cx, NULL, NULL, NULL);
  if (info == NULL ||
      (string = JS_NewStringCopyZ (cx, code)) == NULL ||
      (val = STRING_TO_JSVAL (string)) == 0 ||
      !JS_SetProperty (cx, info, "code", &val) ||
      (string = JS_NewStringCopyZ (cx, level)) == NULL ||
      (val = STRING_TO_JSVAL (string)) == 0 ||
      !JS_SetProperty (cx, info, "level", &val))
    return;
  if (description) {
    if ((string = JS_NewStringCopyZ (cx, level)) == NULL ||
	(val = STRING_TO_JSVAL (string)) == 0 ||
	!JS_SetProperty (cx, info, "description", &val))
      return;
  }
  val = OBJECT_TO_JSVAL (info);
  js_InternalCall (cx, obj, fun, 1, &val, &fun);
}

SwfdecConnection *
swfdec_connection_new (JSContext *cx)
{
  SwfdecConnection *conn;
  SwfdecScriptable *script;

  g_return_val_if_fail (cx != NULL, NULL);

  conn = g_object_new (SWFDEC_TYPE_CONNECTION, NULL);
  script = SWFDEC_SCRIPTABLE (conn);
  script->jscx = cx;

  return conn;
}

void
swfdec_connection_connect (SwfdecConnection *conn, const char *url)
{
  g_return_if_fail (SWFDEC_IS_CONNECTION (conn));

  g_free (conn->url);
  conn->url = g_strdup (url);
  if (url) {
    SWFDEC_ERROR ("FIXME: using NetConnection with non-null URLs is not implemented");
  }
  swfdec_connection_onstatus (conn, "NetConnection.Connect.Success",
      "status", NULL);
}

