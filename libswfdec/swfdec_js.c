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
#include <string.h>
#include <js/jsapi.h>
#include <js/jscntxt.h> /* for setting tracefp when debugging */
#include <js/jsdbgapi.h> /* for debugging */
#include <js/jsopcode.h> /* for debugging */
#include <js/jsscript.h> /* for debugging */
#include "swfdec_types.h"
#include "swfdec_player_internal.h"
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_root_movie.h"
#include "swfdec_swf_decoder.h"

static JSRuntime *swfdec_js_runtime;

/**
 * swfdec_js_init:
 * @runtime_size: desired runtime size of the JavaScript runtime or 0 for default
 *
 * Initializes the Javascript part of swfdec. This function must only be called once.
 **/
void
swfdec_js_init (guint runtime_size)
{
  g_assert (runtime_size < G_MAXUINT32);
  if (runtime_size == 0)
    runtime_size = 8 * 1024 * 1024; /* some default size */

  swfdec_js_runtime = JS_NewRuntime (runtime_size);
  SWFDEC_INFO ("initialized JS runtime with %u bytes", runtime_size);
}

static void
swfdec_js_error_report (JSContext *cx, const char *message, JSErrorReport *report)
{
  SWFDEC_ERROR ("JS Error: %s", message);
  /* FIXME: #ifdef this when not debugging the compiler */
  //g_assert_not_reached ();
}

static JSClass global_class = {
  "global",0,
  JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
  JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
};

static JSTrapStatus G_GNUC_UNUSED
swfdec_js_debug_one (JSContext *cx, JSScript *script, jsbytecode *pc, 
    jsval *rval, void *closure)
{
  if (g_getenv ("SWFDEC_JS") && g_str_equal (g_getenv ("SWFDEC_JS"), "trace"))
    js_Disassemble1 (cx, script, pc, pc - script->code,
	JS_TRUE, stderr);
  return JSTRAP_CONTINUE;
}

/**
 * swfdec_js_init_player:
 * @player: a #SwfdecPlayer
 *
 * Initializes @player for Javascript processing.
 **/
void
swfdec_js_init_player (SwfdecPlayer *player)
{
  player->jscx = JS_NewContext (swfdec_js_runtime, 8192);
  if (player->jscx == NULL) {
    SWFDEC_ERROR ("did not get a JS context, trying to live without");
    return;
  }

  /* the new Flash opcodes mess up this, so this will most likely crash */
  if (g_getenv ("SWFDEC_JS") && g_str_equal (g_getenv ("SWFDEC_JS"), "full"))
    player->jscx->tracefp = stderr;
  JS_SetInterrupt (swfdec_js_runtime, swfdec_js_debug_one, NULL);
  JS_SetErrorReporter (player->jscx, swfdec_js_error_report);
  JS_SetContextPrivate(player->jscx, player);
  player->jsobj = JS_NewObject (player->jscx, &global_class, NULL, NULL);
  if (player->jsobj == NULL) {
    SWFDEC_ERROR ("creating the global object failed");
    swfdec_js_finish_player (player);
    return;
  }
  swfdec_js_add_globals (player);
  if (!JS_InitStandardClasses (player->jscx, player->jsobj)) {
    SWFDEC_ERROR ("initializing JS standard classes failed");
  }
  swfdec_js_add_mouse (player);
  swfdec_js_add_movieclip_class (player);
  swfdec_js_add_color (player);
  swfdec_js_add_sound (player);
}

/**
 * swfdec_js_finish_player:
 * @player: a #SwfdecPlayer
 *
 * Shuts down the Javascript processing for @player.
 **/
void
swfdec_js_finish_player (SwfdecPlayer *player)
{
  if (player->jscx) {
    JS_DestroyContext(player->jscx);
    player->jsobj = NULL;
    player->jscx = NULL;
  }
}

static void
swfdec_disassemble (SwfdecPlayer *player, JSScript *script)
{
  guint i;

  for (i = 0; i < script->length; i ++) {
    g_print ("%02X ", script->code[i]);
    if (i % 16 == 15)
      g_print ("\n");
    else if (i % 4 == 3)
      g_print (" ");
  }
  if (i % 16 != 15)
    g_print ("\n");
  js_Disassemble (player->jscx, script, JS_TRUE, stdout);
}

/**
 * swfdec_js_execute_script:
 * @s: a @SwfdecPlayer
 * @movie: a #SwfdecMovie to pass as argument to the script
 * @script: a @JSScript to execute
 * @rval: optional location for the script's return value
 *
 * Executes the given @script for the given @movie. This function is supposed
 * to be the single entry point for running JavaScript code inswide swfdec, so
 * if you want to execute code, please use this function.
 *
 * Returns: TRUE if the script was successfully executed
 **/
gboolean
swfdec_js_execute_script (SwfdecPlayer *s, SwfdecMovie *movie, 
    JSScript *script, jsval *rval)
{
  jsval returnval = JSVAL_VOID;
  JSObject *jsobj;
  JSBool ret;

  g_return_val_if_fail (s != NULL, FALSE);
  g_return_val_if_fail (SWFDEC_IS_MOVIE (movie), FALSE);
  g_return_val_if_fail (script != NULL, FALSE);

  if (g_getenv ("SWFDEC_JS") && g_str_equal (g_getenv ("SWFDEC_JS"), "disassemble")) {
    g_print ("executing script %p:%p\n", movie, script);
    swfdec_disassemble (s, script);
  }
  if (rval == NULL)
    rval = &returnval;
  if (!(jsobj = swfdec_scriptable_get_object (SWFDEC_SCRIPTABLE (movie))))
    return FALSE;
  ret = JS_ExecuteScript (s->jscx, jsobj, script, rval);
  if (!ret) {
    SWFDEC_WARNING ("executing script %p for movie %s failed", script, movie->name);
  }

  if (ret && returnval != JSVAL_VOID) {
    JSString * str = JS_ValueToString (s->jscx, returnval);
    if (str)
      g_print ("%s\n", JS_GetStringBytes (str));
  }
  return ret ? TRUE : FALSE;
}

/**
 * swfdec_js_run:
 * @player: a #SwfdecPlayer
 * @s: JavaScript commands to execute
 * @rval: optional location to store a return value
 *
 * This is a debugging function for injecting script code into the @player.
 * Use it at your own risk.
 *
 * Returns: TRUE if the script was evaluated successfully. 
 **/
gboolean
swfdec_js_run (SwfdecPlayer *player, const char *s, jsval *rval)
{
  gboolean ret;
  JSScript *script;
  
  g_return_val_if_fail (player != NULL, FALSE);
  g_return_val_if_fail (s != NULL, FALSE);

  script = JS_CompileScript (player->jscx, player->jsobj, s, strlen (s), "injected-code", 1);
  if (script == NULL)
    return FALSE;
  ret = swfdec_js_execute_script (player, 
      SWFDEC_MOVIE (player->movies->data), script, rval);
  JS_DestroyScript (player->jscx, script);
  return ret;
}

/**
 * swfdec_value_to_string:
 * @dec: a #JSContext
 * @val: a #jsval
 *
 * Converts the given jsval to its string representation.
 *
 * Returns: the string representation of @val.
 **/
const char *
swfdec_js_to_string (JSContext *cx, jsval val)
{
  JSString *string;
  char *ret;

  g_return_val_if_fail (cx != NULL, NULL);

  string = JS_ValueToString (cx, val);
  if (string == NULL || (ret = JS_GetStringBytes (string)) == NULL)
    return NULL;

  return ret;
}

/**
 * swfdec_js_slash_to_dot:
 * @slash_str: a string ion slash notation
 *
 * Converts a string in slash notation to a string in dot notation.
 *
 * Returns: The string converted to dot notation or NULL on failure.
 **/
char *
swfdec_js_slash_to_dot (const char *slash_str)
{
  const char *cur = slash_str;
  GString *str = g_string_new ("");

  if (*cur == '/') {
    g_string_append (str, "_root");
  } else {
    goto start;
  }
  while (cur && *cur == '/') {
    cur++;
start:
    if (str->len > 0)
      g_string_append_c (str, '.');
    if (cur[0] == '.' && cur[1] == '.') {
      g_string_append (str, "_parent");
      cur += 2;
    } else {
      char *slash = strchr (cur, '/');
      if (slash) {
	g_string_append_len (str, cur, slash - cur);
	cur = slash;
      } else {
	g_string_append (str, cur);
	cur = NULL;
      }
    }
    /* cur should now point to the slash */
  }
  if (cur) {
    if (*cur != '\0')
      goto fail;
  }
  SWFDEC_DEBUG ("parsed slash-notated string \"%s\" into dot notation \"%s\"",
      slash_str, str->str);
  return g_string_free (str, FALSE);

fail:
  SWFDEC_WARNING ("failed to parse slash-notated string \"%s\" into dot notation", slash_str);
  g_string_free (str, TRUE);
  return NULL;
}

static JSBool
swfdec_js_eval_get_property (JSContext *cx, JSObject *obj, 
    const char *name, guint name_len, jsval *ret)
{
  JSAtom *atom;
  JSObject *pobj;
  JSProperty *prop;

  atom = js_Atomize (cx, name, name_len, 0);
  if (!atom)
    return JS_FALSE;
  if (obj) {
    return OBJ_GET_PROPERTY (cx, obj, (jsid) atom, ret);
  } else {
    if (cx->fp == NULL || cx->fp->scopeChain == NULL)
      return JS_FALSE;
    if (!js_FindProperty (cx, (jsid) atom, &obj, &pobj, &prop))
      return JS_FALSE;
    if (!prop)
      return JS_FALSE;
    return OBJ_GET_PROPERTY (cx, obj, (jsid) prop->id, ret);
  }
}

static JSBool
swfdec_js_eval_set_property (JSContext *cx, JSObject *obj, 
    const char *name, guint name_len, jsval *ret)
{
  JSAtom *atom;

  atom = js_Atomize (cx, name, name_len, 0);
  if (!atom)
    return JS_FALSE;
  if (obj == NULL) {
    if (cx->fp == NULL || cx->fp->scopeChain == NULL)
      return JS_FALSE;
    obj = cx->fp->scopeChain;
  }
  return OBJ_SET_PROPERTY (cx, obj, (jsid) atom, ret);
}

static gboolean
swfdec_js_eval_internal (JSContext *cx, JSObject *obj, const char *str,
        jsval *val, gboolean set)
{
  jsval cur;
  char *work = NULL;

  SWFDEC_LOG ("eval called with \"%s\" on %p", str, obj);
  if (strchr (str, '/')) {
    work = swfdec_js_slash_to_dot (str);
    str = work;
  }
  if (obj == NULL && g_str_has_prefix (str, "this")) {
    str += 4;
    if (*str == '.')
      str++;
    if (cx->fp == NULL)
      goto out;
    obj = cx->fp->thisp;
  }
  cur = OBJECT_TO_JSVAL (obj);
  while (str != NULL && *str != '\0') {
    char *dot = strchr (str, '.');
    if (!JSVAL_IS_OBJECT (cur))
      goto out;
    if (dot) {
      if (!swfdec_js_eval_get_property (cx, obj, str, dot - str, &cur))
	goto out;
      str = dot + 1;
    } else {
      if (set) {
	if (!swfdec_js_eval_set_property (cx, obj, str, strlen (str), val))
	  goto out;
      } else {
	if (!swfdec_js_eval_get_property (cx, obj, str, strlen (str), &cur))
	  goto out;
      }
      goto finish;
    }
    obj = JSVAL_TO_OBJECT (cur);
  }
  if (obj == NULL) {
    if (cx->fp == NULL)
      goto out;
    g_assert (cx->fp->scopeChain);
    cur = OBJECT_TO_JSVAL (OBJ_THIS_OBJECT (cx, cx->fp->scopeChain));
  }

finish:
  g_free (work);
  *val = cur;
  return TRUE;
out:
  SWFDEC_DEBUG ("error: returning void for %s", str);
  g_free (work);
  return FALSE;
}

/**
 * swfdec_js_eval:
 * @cx: a #JSContext
 * @obj: #JSObject to use as a source for evaluating
 * @str: The string to evaluate
 *
 * This function works like the Actionscript eval function used on @obj.
 * It handles both slash-style and dot-style notation.
 *
 * Returns: the value or JSVAL_VOID if no value was found.
 **/
jsval
swfdec_js_eval (JSContext *cx, JSObject *obj, const char *str)
{
  jsval ret;

  g_return_val_if_fail (cx != NULL, JSVAL_VOID);
  g_return_val_if_fail (str != NULL, JSVAL_VOID);

  if (!swfdec_js_eval_internal (cx, obj, str, &ret, FALSE))
    ret = JSVAL_VOID;
  return ret;
}

void
swfdec_js_eval_set (JSContext *cx, JSObject *obj, const char *str,
    jsval val)
{
  g_return_if_fail (cx != NULL);
  g_return_if_fail (str != NULL);

  swfdec_js_eval_internal (cx, obj, str, &val, TRUE);
}
