/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_player_internal.h"
#include "swfdec_as_function.h"
#include "swfdec_as_object.h"
#include "swfdec_debug.h"
#include "swfdec_interval.h"

/*** INTERVALS ***/

static void
swfdec_player_do_set_interval (gboolean repeat, SwfdecAsObject *obj, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (obj->context);
  SwfdecAsObject *object;
  guint id, msecs;
#define MIN_INTERVAL_TIME 10

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0])) {
    SWFDEC_WARNING ("first argument to setInterval is not an object");
    return;
  }
  object = SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]);
  if (SWFDEC_IS_AS_FUNCTION (object)) {
    msecs = swfdec_as_value_to_integer (obj->context, &argv[1]);
    if (msecs < MIN_INTERVAL_TIME) {
      SWFDEC_INFO ("interval duration is %u, making it %u msecs", msecs, MIN_INTERVAL_TIME);
      msecs = MIN_INTERVAL_TIME;
    }
    id = swfdec_interval_new_function (player, msecs, TRUE, 
	SWFDEC_AS_FUNCTION (object), argc - 2, &argv[2]);
  } else {
    const char *name;
    if (argc < 3) {
      SWFDEC_WARNING ("setInterval needs 3 arguments when not called with function");
      return;
    }
    name = swfdec_as_value_to_string (obj->context, &argv[1]);
    msecs = swfdec_as_value_to_integer (obj->context, &argv[2]);
    if (msecs < MIN_INTERVAL_TIME) {
      SWFDEC_INFO ("interval duration is %u, making it %u msecs", msecs, MIN_INTERVAL_TIME);
      msecs = MIN_INTERVAL_TIME;
    }
    id = swfdec_interval_new_object (player, msecs, TRUE, object, name, argc - 3, &argv[3]);
  }
  SWFDEC_AS_VALUE_SET_INT (rval, id);
}

static void
swfdec_player_setInterval (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  swfdec_player_do_set_interval (TRUE, obj, argc, argv, rval);
}

static void
swfdec_player_clearInterval (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (obj->context);
  guint id;
  
  id = swfdec_as_value_to_integer (obj->context, &argv[0]);
  swfdec_interval_remove (player, id);
}

/*** VARIOUS ***/

#if 0
void
swfdec_js_global_eval (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  if (JSVAL_IS_STRING (argv[0])) {
    const char *bytes = swfdec_js_to_string (cx, argv[0]);
    if (bytes == NULL)
      return JS_FALSE;
    *rval = swfdec_js_eval (cx, obj, bytes);
  } else {
    *rval = argv[0];
  }
  return JS_TRUE;
}

static void
swfdec_js_trace (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);
  const char *bytes;

  bytes = swfdec_js_to_string (cx, argv[0]);
  if (bytes == NULL)
    return JS_TRUE;

  swfdec_player_trace (player, bytes);
  return JS_TRUE;
}

static void
swfdec_js_random (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  gint32 max, result;

  if (!JS_ValueToECMAInt32 (cx, argv[0], &max))
    return JS_FALSE;
  
  if (max <= 0)
    result = 0;
  else
    result = g_random_int_range (0, max);

  return JS_NewNumberValue(cx, result, rval);
}

static void
swfdec_js_stopAllSounds (SwfdecAsObject *obj, guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);

  swfdec_player_stop_all_sounds (player);
  return JS_TRUE;
}

static JSFunctionSpec global_methods[] = {
  { "clearInterval",	swfdec_js_global_clearInterval,	1, 0, 0 },
  { "eval",		swfdec_js_global_eval,		1, 0, 0 },
  { "random",		swfdec_js_random,		1, 0, 0 },
  { "setInterval",	swfdec_js_global_setInterval,	2, 0, 0 },
  { "stopAllSounds",	swfdec_js_stopAllSounds,	0, 0, 0 },
  { "trace",     	swfdec_js_trace,		1, 0, 0 },
  { NULL, NULL, 0, 0, 0 }
};
#endif

static void
swfdec_player_object_registerClass (SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *name;
  
  name = swfdec_as_value_to_string (object->context, &argv[0]);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[1])) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
    return;
  }
  
  swfdec_player_set_export_class (SWFDEC_PLAYER (object->context), name, 
      SWFDEC_AS_VALUE_GET_OBJECT (&argv[1]));
  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}

void
swfdec_player_init_global (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context = SWFDEC_AS_CONTEXT (player);

  swfdec_as_object_add_function (context->Object, SWFDEC_AS_STR_registerClass, 
      0, swfdec_player_object_registerClass, 2);
  swfdec_as_object_add_function (context->global, SWFDEC_AS_STR_setInterval, 
      0, swfdec_player_setInterval, 2);
  swfdec_as_object_add_function (context->global, SWFDEC_AS_STR_clearInterval, 
      0, swfdec_player_clearInterval, 1);
}

