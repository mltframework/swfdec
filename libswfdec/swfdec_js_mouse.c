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
#include "swfdec_listener.h"
#include "swfdec_player_internal.h"

static JSBool
swfdec_js_mouse_add_listener (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);

  g_assert (player);
  if (!JSVAL_IS_OBJECT (argv[0]) || argv[0] == JSVAL_NULL)
    return JS_TRUE;
  return swfdec_listener_add (player->mouse_listener, JSVAL_TO_OBJECT (argv[0]));
}

static JSBool
swfdec_js_mouse_remove_listener (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);

  g_assert (player);
  if (!JSVAL_IS_OBJECT (argv[0]) || argv[0] == JSVAL_NULL)
    return JS_TRUE;
  swfdec_listener_remove (player->mouse_listener, JSVAL_TO_OBJECT (argv[0]));
  return JS_TRUE;
}

static JSBool
swfdec_js_mouse_show (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);

  g_assert (player);
  player->mouse_visible = TRUE;
  return JS_TRUE;
}

static JSBool
swfdec_js_mouse_hide (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);

  g_assert (player);
  player->mouse_visible = FALSE;
  return JS_TRUE;
}

static JSFunctionSpec mouse_methods[] = {
    {"addListener",   	swfdec_js_mouse_add_listener,		1, 0, 0 },
    {"hide",		swfdec_js_mouse_hide,			0, 0, 0 },
    {"removeListener",	swfdec_js_mouse_remove_listener,	1, 0, 0 },
    {"show",		swfdec_js_mouse_show,			0, 0, 0 },
    {0,0,0,0,0}
};

static JSClass mouse_class = {
    "Mouse",
    0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

void
swfdec_js_add_mouse (SwfdecPlayer *player)
{
  JSObject *mouse;
  
  mouse = JS_DefineObject(player->jscx, player->jsobj, "Mouse", &mouse_class, NULL, 0);
  if (!mouse || 
      !JS_DefineFunctions(player->jscx, mouse, mouse_methods)) {
    SWFDEC_ERROR ("failed to initialize Mouse object");
  }
}

