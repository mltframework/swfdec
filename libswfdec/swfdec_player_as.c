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

/* NB: include this first, it redefines SWFDEC_AS_NATIVE */
#include "swfdec_asnative.h"

#include "swfdec_player_internal.h"
#include "swfdec_as_function.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_interval.h"
/* FIXME: to avoid duplicate definitions */
#undef SWFDEC_AS_NATIVE
#define SWFDEC_AS_NATIVE(x, y, func)

/*** INTERVALS ***/

static void
swfdec_player_do_set_interval (gboolean repeat, SwfdecAsContext *cx, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  SwfdecAsObject *object;
  guint id, msecs;
#define MIN_INTERVAL_TIME 10

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0])) {
    SWFDEC_WARNING ("first argument to setInterval is not an object");
    return;
  }
  object = SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]);
  if (SWFDEC_IS_AS_FUNCTION (object)) {
    msecs = swfdec_as_value_to_integer (cx, &argv[1]);
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
    name = swfdec_as_value_to_string (cx, &argv[1]);
    msecs = swfdec_as_value_to_integer (cx, &argv[2]);
    if (msecs < MIN_INTERVAL_TIME) {
      SWFDEC_INFO ("interval duration is %u, making it %u msecs", msecs, MIN_INTERVAL_TIME);
      msecs = MIN_INTERVAL_TIME;
    }
    id = swfdec_interval_new_object (player, msecs, TRUE, object, name, argc - 3, &argv[3]);
  }
  SWFDEC_AS_VALUE_SET_INT (rval, id);
}

static void
swfdec_player_setInterval (SwfdecAsContext *cx, SwfdecAsObject *obj, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  swfdec_player_do_set_interval (TRUE, cx, argc, argv, rval);
}

static void
swfdec_player_clearInterval (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  guint id;
  
  id = swfdec_as_value_to_integer (cx, &argv[0]);
  swfdec_interval_remove (player, id);
}

/*** VARIOUS ***/

static SwfdecAsFunction *
swfdec_get_asnative (SwfdecAsContext *cx, guint x, guint y)
{
  guint i;

  for (i = 0; native_funcs[i].func != NULL; i++) {
    if (native_funcs[i].x == x && native_funcs[i].y == y) {
      SwfdecAsFunction *fun = swfdec_as_native_function_new (cx, native_funcs[i].name,
	  native_funcs[i].func, 0, NULL);
      if (native_funcs[i].get_type) {
	swfdec_as_native_function_set_construct_type (SWFDEC_AS_NATIVE_FUNCTION (fun),
	    native_funcs[i].get_type ());
      }
      return fun;
    }
  }
  return NULL;
}

// same as ASnative, but also sets prototype
static void
swfdec_player_ASconstructor (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsValue val;
  SwfdecAsObject *proto;
  SwfdecAsFunction *func;
  guint x, y;

  x = swfdec_as_value_to_integer (cx, &argv[0]);
  y = swfdec_as_value_to_integer (cx, &argv[1]);

  func = swfdec_get_asnative (cx, x, y);
  if (func) {
    proto = swfdec_as_object_new (cx);

    SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
    swfdec_as_object_set_variable_and_flags (SWFDEC_AS_OBJECT (func),
	SWFDEC_AS_STR_prototype, &val,
	SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

    SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (func));
    swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR_constructor,
	&val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

    SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (func));
  } else {
    SWFDEC_FIXME ("ASconstructor for %u %u missing", x, y);
  }
}

static void
swfdec_player_ASnative (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsFunction *func;
  guint x, y;

  x = swfdec_as_value_to_integer (cx, &argv[0]);
  y = swfdec_as_value_to_integer (cx, &argv[1]);

  func = swfdec_get_asnative (cx, x, y);
  if (func) {
    SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (func));
  } else {
    SWFDEC_FIXME ("ASnative for %u %u missing", x, y);
  }
}

SWFDEC_AS_NATIVE (4, 0, ASSetNative)
void
ASSetNative (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsFunction *function;
  SwfdecAsObject *target;
  SwfdecAsValue val;
  const char *s;
  char **names;
  guint i, x, y;

  if (argc < 3)
    return;

  target = swfdec_as_value_to_object (cx, &argv[0]);
  x = swfdec_as_value_to_integer (cx, &argv[1]);
  s = swfdec_as_value_to_string (cx, &argv[2]);
  if (argc > 3)
    y = swfdec_as_value_to_integer (cx, &argv[3]);
  else
    y = 0;
  names = g_strsplit (s, ",", -1);
  for (i = 0; names[i]; i++) {
    s = names[i];
    if (s[0] >= '0' && s[0] <= '9') {
      SWFDEC_FIXME ("implement the weird numbers");
      s++;
    }
    function = swfdec_get_asnative (cx, x, y);
    if (function == NULL) {
      SWFDEC_FIXME ("no ASnative function for %u, %u, what now?", x, y);
      break;
    }
    SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (function));
    swfdec_as_object_set_variable (target, swfdec_as_context_get_string (cx, s), &val);
    y++;
  }
  g_strfreev (names);
}

SWFDEC_AS_NATIVE (4, 1, ASSetNativeAccessor)
void
ASSetNativeAccessor (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsFunction *get, *set;
  SwfdecAsObject *target;
  const char *s;
  char **names;
  guint i, x, y;

  if (argc < 3)
    return;

  target = swfdec_as_value_to_object (cx, &argv[0]);
  x = swfdec_as_value_to_integer (cx, &argv[1]);
  s = swfdec_as_value_to_string (cx, &argv[2]);
  if (argc > 3)
    y = swfdec_as_value_to_integer (cx, &argv[3]);
  else
    y = 0;
  names = g_strsplit (s, ",", -1);
  for (i = 0; names[i]; i++) {
    s = names[i];
    if (s[0] >= '0' && s[0] <= '9') {
      SWFDEC_FIXME ("implement the weird numbers");
      s++;
    }
    get = swfdec_get_asnative (cx, x, y++);
    set = swfdec_get_asnative (cx, x, y++);
    if (get == NULL) {
      SWFDEC_ERROR ("no getter for %s", s);
      break;
    }
    swfdec_as_object_add_variable (target, swfdec_as_context_get_string (cx, s),
	get, set);
  }
  g_strfreev (names);
}

static void
swfdec_player_object_registerClass (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *name;
  
  name = swfdec_as_value_to_string (cx, &argv[0]);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[1])) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
    return;
  }
  
  swfdec_player_set_export_class (SWFDEC_PLAYER (cx), name, 
      SWFDEC_AS_VALUE_GET_OBJECT (&argv[1]));
  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}

// this is ran before swfdec_as_context_startup
void
swfdec_player_preinit_global (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context = SWFDEC_AS_CONTEXT (player);

  // init these two before swfdec_as_context_startup, so they won't get
  // __proto__ and constructor properties
  swfdec_as_object_add_function (context->global, SWFDEC_AS_STR_ASnative, 
      0, swfdec_player_ASnative, 2);
  swfdec_as_object_add_function (context->global, SWFDEC_AS_STR_ASconstructor,
      0, swfdec_player_ASconstructor, 2);
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

