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
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_interval.h"
#include "swfdec_as_frame_internal.h"

/*** INTERVALS ***/

static void
swfdec_player_do_set_interval (gboolean repeat, SwfdecAsContext *cx, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  SwfdecAsObject *object;
  guint id, msecs;
#define MIN_INTERVAL_TIME 10

  if (argc < 2) {
    SWFDEC_WARNING ("setInterval needs at least 2 arguments");
    return;
  }

  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (&argv[0]) ||
      (object = SWFDEC_AS_VALUE_GET_COMPOSITE (&argv[0])) == NULL) {
    SWFDEC_WARNING ("first argument to setInterval is not an object");
    return;
  }
  if (SWFDEC_IS_AS_FUNCTION (object->relay)) {
    msecs = swfdec_as_value_to_integer (cx, &argv[1]);
    if (msecs < MIN_INTERVAL_TIME) {
      SWFDEC_INFO ("interval duration is %u, making it %u msecs", msecs, MIN_INTERVAL_TIME);
      msecs = MIN_INTERVAL_TIME;
    }
    id = swfdec_interval_new_function (player, msecs, repeat, 
	SWFDEC_AS_FUNCTION (object->relay), argc - 2, &argv[2]);
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
    id = swfdec_interval_new_object (player, msecs, repeat, &argv[0], name, argc - 3, &argv[3]);
  }
  swfdec_as_value_set_integer (cx, rval, id);
}

SWFDEC_AS_NATIVE (2, 0, swfdec_player_ASnew)
void
swfdec_player_ASnew (SwfdecAsContext *cx, SwfdecAsObject *obj, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  g_return_if_fail (cx->frame->next != NULL);

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, cx->frame->next->construct);
}

SWFDEC_AS_NATIVE (250, 0, swfdec_player_setInterval)
void
swfdec_player_setInterval (SwfdecAsContext *cx, SwfdecAsObject *obj, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  swfdec_player_do_set_interval (TRUE, cx, argc, argv, rval);
}

SWFDEC_AS_NATIVE (250, 2, swfdec_player_setTimeout)
void
swfdec_player_setTimeout (SwfdecAsContext *cx, SwfdecAsObject *obj, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  swfdec_player_do_set_interval (FALSE, cx, argc, argv, rval);
}

SWFDEC_AS_NATIVE (250, 1, swfdec_player_clearInterval)
void
swfdec_player_clearInterval (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  guint id;

  SWFDEC_AS_CHECK (0, NULL, "i", &id);
  
  swfdec_interval_remove (player, id);
}

/*** VARIOUS ***/

SWFDEC_AS_NATIVE (100, 4, swfdec_player_trace)
void
swfdec_player_trace (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_FIXME ("Is _global.trace supposed to do something?");
}

SWFDEC_AS_NATIVE (9, 0, swfdec_player_updateAfterEvent)
void
swfdec_player_updateAfterEvent (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("updateAfterEvent");
}

SWFDEC_AS_NATIVE (1021, 1, swfdec_player_showRedrawRegions)
void
swfdec_player_showRedrawRegions (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("showRedrawRegions");
}

static void
swfdec_player_enableDebugConsole (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("enableDebugConsole");
}

static void
swfdec_player_do_nothing (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
}

static SwfdecAsFunction *
swfdec_get_asnative (SwfdecAsContext *cx, guint x, guint y)
{
  gboolean x_exists;
  guint i;

  x_exists = FALSE;
  for (i = 0; native_funcs[i].func != NULL; i++) {
    if (native_funcs[i].x == x)
      x_exists = TRUE;
    if (native_funcs[i].x == x && native_funcs[i].y == y) {
      SwfdecAsFunction *fun = swfdec_as_native_function_new (cx, native_funcs[i].name,
	  native_funcs[i].func);
      return fun;
    }
  }
  SWFDEC_WARNING ("no ASnative (%u, %u)", x, y);
  if (x_exists) {
    SwfdecAsFunction *func;
    char *name = g_strdup_printf ("ASnative (%u, %u)", x, y);
    func = swfdec_as_native_function_new (cx, name, swfdec_player_do_nothing);
    g_free (name);
    return func;
  } else {
    return NULL;
  }
}

// same as ASnative, but also sets prototype
static void
swfdec_player_ASconstructor (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsValue val;
  SwfdecAsObject *proto, *func_object;
  SwfdecAsFunction *func;
  guint x, y;

  SWFDEC_AS_CHECK (0, NULL, "ii", &x, &y);

  func = swfdec_get_asnative (cx, x, y);
  if (func) {
    proto = swfdec_as_object_new (cx, SWFDEC_AS_STR_Object, NULL);
    func_object = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (func));

    SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
    swfdec_as_object_set_variable_and_flags (func_object,
	SWFDEC_AS_STR_prototype, &val,
	SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

    SWFDEC_AS_VALUE_SET_OBJECT (&val, func_object);
    swfdec_as_object_set_variable_and_flags (proto, SWFDEC_AS_STR_constructor,
	&val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

    SWFDEC_AS_VALUE_SET_OBJECT (rval, func_object);
  }
}

static void
swfdec_player_ASnative (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsFunction *func;
  guint x, y;

  SWFDEC_AS_CHECK (0, NULL, "ii", &x, &y);

  func = swfdec_get_asnative (cx, x, y);
  if (func) {
    SWFDEC_AS_VALUE_SET_OBJECT (rval, swfdec_as_relay_get_as_object SWFDEC_AS_RELAY (func));
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
  SwfdecAsVariableFlag flags;
  const char *s;
  char **names;
  guint i, x, y;

  SWFDEC_AS_CHECK (0, NULL, "ois", &target, &x, &s);

  if (argc > 3)
    y = swfdec_as_value_to_integer (cx, &argv[3]);
  else
    y = 0;
  names = g_strsplit (s, ",", -1);
  for (i = 0; names[i]; i++) {
    s = names[i];
    flags = 0;
    if (s[0] == '6') {
      flags |= SWFDEC_AS_VARIABLE_VERSION_6_UP;
      s++;
    } else if (s[0] == '7') {
      flags |= SWFDEC_AS_VARIABLE_VERSION_7_UP;
      s++;
    } else if (s[0] == '8') {
      flags |= SWFDEC_AS_VARIABLE_VERSION_8_UP;
      s++;
    } else if (s[0] == '9') {
      flags |= SWFDEC_AS_VARIABLE_VERSION_9_UP;
      s++;
    }
    function = swfdec_get_asnative (cx, x, y);
    if (function == NULL)
      break;
    SWFDEC_AS_VALUE_SET_OBJECT (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (function)));
    swfdec_as_object_set_variable_and_flags (target,
	swfdec_as_context_get_string (cx, s), &val, flags);
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
  SwfdecAsVariableFlag flags;
  const char *s;
  char **names;
  guint i, x, y = 0;

  SWFDEC_AS_CHECK (0, NULL, "ois|i", &target, &x, &s, &y);

  names = g_strsplit (s, ",", -1);
  for (i = 0; names[i]; i++) {
    s = names[i];
    flags = 0;
    if (s[0] == '6') {
      flags |= SWFDEC_AS_VARIABLE_VERSION_6_UP;
      s++;
    } else if (s[0] == '7') {
      flags |= SWFDEC_AS_VARIABLE_VERSION_7_UP;
      s++;
    } else if (s[0] == '8') {
      flags |= SWFDEC_AS_VARIABLE_VERSION_8_UP;
      s++;
    } else if (s[0] == '9') {
      flags |= SWFDEC_AS_VARIABLE_VERSION_9_UP;
      s++;
    }
    get = swfdec_get_asnative (cx, x, y++);
    set = swfdec_get_asnative (cx, x, y++);
    if (get == NULL) {
      SWFDEC_ERROR ("no getter for %s", s);
      break;
    }
    swfdec_as_object_add_variable (target, swfdec_as_context_get_string (cx, s),
	get, set, flags);
  }
  g_strfreev (names);
}

SWFDEC_AS_NATIVE (101, 8, swfdec_player_object_registerClass)
void
swfdec_player_object_registerClass (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *name;

  SWFDEC_AS_CHECK (0, NULL, "s", &name);

  if (argc < 2 || !SWFDEC_AS_VALUE_IS_OBJECT (&argv[1])) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
    return;
  }
  
  swfdec_player_set_export_class (SWFDEC_PLAYER (cx), name, 
      SWFDEC_AS_VALUE_GET_OBJECT (&argv[1]));
  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}

/* This is ran at the beginning of swfdec_as_context_startup.
 * Yes, this is a hack */
void
swfdec_player_preinit_global (SwfdecAsContext *context)
{
  SwfdecAsObject *o;
  SwfdecAsFunction *f;
  SwfdecAsValue val;

  f = swfdec_as_native_function_new_bare (context,
	SWFDEC_AS_STR_ASnative, swfdec_player_ASnative);
  o = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (f));
  SWFDEC_AS_VALUE_SET_OBJECT (&val, o);
  swfdec_as_object_set_variable_and_flags (context->global, SWFDEC_AS_STR_ASnative,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

  f = swfdec_as_native_function_new_bare (context,
	SWFDEC_AS_STR_ASconstructor, swfdec_player_ASconstructor);
  o = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (f));
  SWFDEC_AS_VALUE_SET_OBJECT (&val, o);
  swfdec_as_object_set_variable_and_flags (context->global, SWFDEC_AS_STR_ASconstructor,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

  f = swfdec_as_native_function_new_bare (context,
	SWFDEC_AS_STR_enableDebugConsole, swfdec_player_enableDebugConsole);
  o = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (f));
  SWFDEC_AS_VALUE_SET_OBJECT (&val, o);
  swfdec_as_object_set_variable_and_flags (context->global, SWFDEC_AS_STR_enableDebugConsole,
      &val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
}
