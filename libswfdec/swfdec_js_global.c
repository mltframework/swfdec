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

#include <string.h>
#include "swfdec_js.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "js/jsatom.h"
#include "js/jsfun.h"
#include "js/jsinterp.h"
#include "js/jsobj.h"

/*** INTERVAL ***/

typedef struct _SwfdecJSInterval SwfdecJSInterval;
struct _SwfdecJSInterval {
  SwfdecTimeout		timeout;
  SwfdecPlayer *	player;		/* needed so it can be readded */
  unsigned int		id;		/* id this interval is identified with */
  unsigned int		msecs;		/* interval in milliseconds */
  unsigned int		n_args;		/* number of arguments to call function with */
  jsval			vals[0];	/* values: 0 is function, 1 is object, 2-n are arguments */
};

void
swfdec_js_interval_free (SwfdecJSInterval *interval)
{
  JSContext *cx = interval->player->jscx;
  guint i;

  swfdec_player_remove_timeout (interval->player, &interval->timeout);
  interval->player->intervals = 
    g_list_remove (interval->player->intervals, interval);
  for (i = 0; i < interval->n_args + 2; i++) {
    JS_RemoveRoot (cx, &interval->vals[i]);
  }
  g_free (interval);
}

static void
swfdec_js_interval_trigger (SwfdecTimeout *timeout)
{
  SwfdecJSInterval *interval = (SwfdecJSInterval *) timeout;
  JSContext *cx = interval->player->jscx;
  jsval fun, rval;

  timeout->timestamp += SWFDEC_MSECS_TO_TICKS (interval->msecs);
  swfdec_player_add_timeout (interval->player, timeout);
  g_assert (JSVAL_IS_OBJECT (interval->vals[1]));
  if (JSVAL_IS_STRING (interval->vals[0])) {
    JSAtom *atom = js_AtomizeString (cx, JSVAL_TO_STRING (interval->vals[0]), 0);
    if (!atom)
      return;
    if (!js_GetProperty (cx, JSVAL_TO_OBJECT (interval->vals[1]),
	  (jsid) atom, &fun))
      return;
  } else {
    fun = interval->vals[0];
  }
  js_InternalCall (cx, JSVAL_TO_OBJECT (interval->vals[1]), fun,
      interval->n_args, &interval->vals[2], &rval);
}

static SwfdecJSInterval *
swfdec_js_interval_new (guint n_args)
{
  SwfdecJSInterval *ret = g_malloc (sizeof (SwfdecJSInterval) + sizeof (jsval) * (2 + n_args));

  ret->timeout.callback = swfdec_js_interval_trigger;
  ret->n_args = n_args;
  return ret;
}

static JSBool
swfdec_js_global_setInterval (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);
  JSObject *object;
  jsval fun;
  unsigned int i, n_args, first_arg, msecs;
  SwfdecJSInterval *interval;

  if (!JSVAL_IS_OBJECT (argv[0])) {
    SWFDEC_WARNING ("first argument to setInterval is not an object");
    return JS_TRUE;
  }
  object = JSVAL_TO_OBJECT (argv[0]);
  if (JS_GetClass (object) == &js_FunctionClass) {
    fun = argv[0];
    object = JS_GetParent (cx, object);
    if (object == NULL) {
      SWFDEC_WARNING ("function has no parent?!");
      return JS_TRUE;
    }
    first_arg = 2;
  } else {
    if (argc < 3) {
      SWFDEC_WARNING ("setInterval needs 3 arguments when not called with function");
      return JS_TRUE;
    }
    if (!JSVAL_IS_STRING (argv[1])) {
      SWFDEC_WARNING ("function name passed to setInterval is not a string");
      return JS_TRUE;
    }
    fun = argv[1];
    first_arg = 3;
  }
  if (!JS_ValueToECMAUint32 (cx, argv[first_arg - 1], &msecs))
    return JS_FALSE;
#define MIN_INTERVAL_TIME 10
  if (msecs < MIN_INTERVAL_TIME) {
    SWFDEC_INFO ("interval duration is %u, making it %u msecs", msecs, MIN_INTERVAL_TIME);
    msecs = MIN_INTERVAL_TIME;
  }
  n_args = argc - first_arg;
  interval = swfdec_js_interval_new (n_args);
  interval->player = player;
  interval->id = ++player->interval_id;
  interval->msecs = msecs;
  interval->vals[0] = fun;
  interval->vals[1] = OBJECT_TO_JSVAL (object);
  memcpy (&interval->vals[2], &argv[first_arg], n_args);
  for (i = 0; i < n_args + 2; i++) {
    if (!JS_AddRoot (cx, &interval->vals[i])) {
      /* FIXME: is it save roots that weren't added before? */
      swfdec_js_interval_free (interval);
      return JS_FALSE;
    }
  }
  interval->timeout.timestamp = player->time + SWFDEC_MSECS_TO_TICKS (interval->msecs);
  swfdec_player_add_timeout (player, &interval->timeout);
  interval->player->intervals = 
    g_list_prepend (interval->player->intervals, interval);
  *rval = INT_TO_JSVAL (interval->id);
  return JS_TRUE;
}

static JSBool
swfdec_js_global_clearInterval (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);
  guint id;
  GList *walk;

  if (!JSVAL_IS_INT (argv[0])) {
    SWFDEC_WARNING ("argument is not an int");
    return JS_TRUE;
  }
  id = JSVAL_TO_INT (argv[0]);
  for (walk = player->intervals; walk; walk = walk->next) {
    SwfdecJSInterval *interval = walk->data;
    if (interval->id != id)
      continue;
    swfdec_js_interval_free (interval);
    break;
  }
  return JS_TRUE;
}

/*** VARIOUS ***/

JSBool
swfdec_js_global_eval (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
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

static JSBool
swfdec_js_trace (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);
  const char *bytes;

  bytes = swfdec_js_to_string (cx, argv[0]);
  if (bytes == NULL)
    return JS_TRUE;

  swfdec_player_trace (player, bytes);
  return JS_TRUE;
}

static JSBool
swfdec_js_random (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
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

static JSBool
swfdec_js_stopAllSounds (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
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

void
swfdec_js_add_globals (SwfdecPlayer *player)
{
  if (!JS_DefineFunctions (player->jscx, player->jsobj, global_methods)) {
    SWFDEC_ERROR ("failed to initialize global methods");
  }
}

