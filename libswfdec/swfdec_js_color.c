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
swfdec_js_color_get_rgb (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  int result;
  SwfdecMovie *movie = JS_GetPrivate (cx, obj);

  g_assert (movie);
  result = (movie->color_transform.rb << 16) |
	   ((movie->color_transform.gb % 256) << 8) | 
	   (movie->color_transform.bb % 256);
  *rval = INT_TO_JSVAL (result);
  return JS_TRUE;
}

static inline void
add_variable (JSContext *cx, JSObject *obj, const char *name, double value)
{
  jsval val;

  if (JS_NewNumberValue (cx, value, &val))
    JS_SetProperty (cx, obj, name, &val);
}

static JSBool
swfdec_js_color_get_transform (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  JSObject *ret;
  SwfdecMovie *movie = JS_GetPrivate (cx, obj);

  g_assert (movie);
  ret = JS_NewObject (cx, NULL, NULL, NULL);
  if (ret == NULL)
    return JS_TRUE;

  add_variable (cx, ret, "ra", movie->color_transform.ra * 100.0 / 256.0);
  add_variable (cx, ret, "ga", movie->color_transform.ga * 100.0 / 256.0);
  add_variable (cx, ret, "ba", movie->color_transform.ba * 100.0 / 256.0);
  add_variable (cx, ret, "aa", movie->color_transform.aa * 100.0 / 256.0);
  add_variable (cx, ret, "rb", movie->color_transform.rb);
  add_variable (cx, ret, "gb", movie->color_transform.gb);
  add_variable (cx, ret, "bb", movie->color_transform.bb);
  add_variable (cx, ret, "ab", movie->color_transform.ab);
  *rval = OBJECT_TO_JSVAL (ret);
  return JS_TRUE;
}

static JSBool
swfdec_js_color_set_rgb (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  unsigned int color;
  SwfdecMovie *movie = JS_GetPrivate (cx, obj);

  g_assert (movie);
  if (!JS_ValueToECMAUint32 (cx, argv[0], &color))
    return JS_TRUE;

  movie->color_transform.ra = 0;
  movie->color_transform.rb = (color & 0xFF0000) >> 16;
  movie->color_transform.ga = 0;
  movie->color_transform.gb = (color & 0xFF00) >> 8;
  movie->color_transform.ba = 0;
  movie->color_transform.bb = color & 0xFF;
  swfdec_movie_invalidate (movie);
  return JS_TRUE;
}

static inline void
parse_property (JSContext *cx, JSObject *obj, const char *name, int *target, gboolean scale)
{
  jsval val;
  double d;

  if (!JS_GetProperty (cx, obj, name, &val))
    return;
  if (JSVAL_IS_VOID (val))
    return;
  if (!JS_ValueToNumber (cx, val, &d))
    return;
  if (scale) {
    *target = d * 256.0 / 100.0;
  } else {
    *target = d;
  }
}

static JSBool
swfdec_js_color_set_transform (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  JSObject *parse;
  SwfdecMovie *movie = JS_GetPrivate (cx, obj);

  g_assert (movie);
  if (!JSVAL_IS_OBJECT (argv[0]))
    return JS_TRUE;
  parse = JSVAL_TO_OBJECT (argv[0]);
  parse_property (cx, parse, "ra", &movie->color_transform.ra, TRUE);
  parse_property (cx, parse, "ga", &movie->color_transform.ga, TRUE);
  parse_property (cx, parse, "ba", &movie->color_transform.ba, TRUE);
  parse_property (cx, parse, "aa", &movie->color_transform.aa, TRUE);
  parse_property (cx, parse, "rb", &movie->color_transform.rb, FALSE);
  parse_property (cx, parse, "gb", &movie->color_transform.gb, FALSE);
  parse_property (cx, parse, "bb", &movie->color_transform.bb, FALSE);
  parse_property (cx, parse, "ab", &movie->color_transform.ab, FALSE);
  swfdec_movie_invalidate (movie);
  return JS_TRUE;
}

static JSBool
swfdec_js_color_to_string (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  JSString *string;

  string = JS_InternString (cx, "[object Object]");
  if (string == NULL)
    return JS_FALSE;

  *rval = STRING_TO_JSVAL (string);
  return JS_TRUE;
}

static JSFunctionSpec color_methods[] = {
  { "getRGB",		swfdec_js_color_get_rgb,	1, 0, 0 },
  { "getTransform",  	swfdec_js_color_get_transform,	1, 0, 0 },
  { "setRGB",		swfdec_js_color_set_rgb,	1, 0, 0 },
  { "setTransform",  	swfdec_js_color_set_transform,	1, 0, 0 },
  { "toString",	  	swfdec_js_color_to_string,	0, 0, 0 },
  {0,0,0,0,0}
};

static void
swfdec_js_color_finalize (JSContext *cx, JSObject *obj)
{
  SwfdecMovie *movie;

  movie = JS_GetPrivate (cx, obj);
  /* since we also finalize the class, not everyone has a private object */
  if (movie) {
    g_object_unref (movie);
  }
}

static JSClass color_class = {
    "Color", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   swfdec_js_color_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
swfdec_js_color_new (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovie *movie;
  JSObject *new;

  movie = swfdec_js_val_to_movie (cx, argv[0]);
  if (movie == NULL) {
    SWFDEC_INFO ("attempted to construct a color without a movie");
    return JS_TRUE;
  }
  new = JS_NewObject (cx, &color_class, NULL, NULL);
  if (new == NULL)
    return JS_TRUE;
  if (!JS_SetPrivate (cx, new, movie))
    return JS_TRUE;
  g_object_ref (movie);
  *rval = OBJECT_TO_JSVAL (new);
  return JS_TRUE;
}

void
swfdec_js_add_color (SwfdecPlayer *player)
{
  JS_InitClass (player->jscx, player->jsobj, NULL,
      &color_class, swfdec_js_color_new, 1, NULL, color_methods,
      NULL, NULL);
}

