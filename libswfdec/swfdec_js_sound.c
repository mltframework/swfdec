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
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"

static JSBool
not_implemented (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SWFDEC_WARNING ("FIXME: implement this sound function");
  return JS_TRUE;
}

static JSFunctionSpec sound_methods[] = {
  { "attachSound",	  	not_implemented,	1, 0, 0 },
  { "getPan",		  	not_implemented,	0, 0, 0 },
  { "getTransform",	  	not_implemented,	0, 0, 0 },
  { "getVolume",	  	not_implemented,	0, 0, 0 },
  { "setPan",		  	not_implemented,	1, 0, 0 },
  { "setTransform",	  	not_implemented,	1, 0, 0 },
  { "setVolume",	  	not_implemented,	1, 0, 0 },
  { "start",		  	not_implemented,	0, 0, 0 },
  { "stop",		  	not_implemented,	0, 0, 0 },
  {0,0,0,0,0}
};

static void
swfdec_js_sound_finalize (JSContext *cx, JSObject *obj)
{
}

static JSClass sound_class = {
    "Sound", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   swfdec_js_sound_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
swfdec_js_sound_new (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  *rval = OBJECT_TO_JSVAL (obj);
  return JS_TRUE;
}

void
swfdec_js_add_sound (SwfdecPlayer *player)
{
  JS_InitClass (player->jscx, player->jsobj, NULL,
      &sound_class, swfdec_js_sound_new, 1, NULL, sound_methods,
      NULL, NULL);
}

