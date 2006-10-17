/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_js.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

static JSBool
swfdec_js_trace (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);
  const char *bytes;

  bytes = swfdec_js_to_string (cx, argv[0]);
  if (bytes == NULL)
    return JS_TRUE;

  /* FIXME: accumulate and emit after JS handling? */
  g_signal_emit_by_name (player, "trace", bytes);
  return JS_TRUE;
}

static JSBool
swfdec_js_random (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  gint32 max, result;

  if (!JS_ValueToECMAInt32 (cx, argv[0], &max))
    return JS_FALSE;
  
  result = g_random_int_range (0, max);

  return JS_NewNumberValue(cx, result, rval);
}

static JSBool
swfdec_js_startDrag (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  return JS_TRUE;
}

static JSFunctionSpec global_methods[] = {
  { "random",		swfdec_js_random,	1, 0, 0 },
  { "startDrag",     	swfdec_js_startDrag,	1, 0, 0 },
  { "trace",     	swfdec_js_trace,	1, 0, 0 },
  { NULL, NULL, 0, 0, 0 }
};

void
swfdec_js_add_globals (SwfdecPlayer *player)
{
  if (!JS_DefineFunctions (player->jscx, player->jsobj, global_methods)) {
    SWFDEC_ERROR ("failed to initialize global methods");
  }
}

