
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
#include "swfdec_decoder.h"
#include "swfdec_movieclip.h"
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_compiler.h"

static JSRuntime *swfdec_js_runtime;

typedef struct {
  SwfdecMovieClip *	movie;
  JSScript *		script;
} SwfdecScriptEntry;

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
  SWFDEC_ERROR ("JS Error: %s\n", message);
  /* FIXME: #ifdef this when not debugging the compiler */
  g_assert_not_reached ();
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
  js_Disassemble1 (cx, script, pc, pc - script->code,
      JS_TRUE, stderr);
  return JSTRAP_CONTINUE;
}

/**
 * swfdec_js_init_decoder:
 * @s: a #SwfdecDecoder
 *
 * Initializes @s for Javascript processing.
 **/
void
swfdec_js_init_decoder (SwfdecDecoder *s)
{
  JSObject *glob;

  s->jscx = JS_NewContext (swfdec_js_runtime, 8192);
  if (s->jscx == NULL) {
    SWFDEC_ERROR ("did not get a JS context, trying to live without");
    return;
  }

#if 0
  /* the new opcodes mess up this */
  s->jscx->tracefp = stderr;
#endif
#if 0
  JS_SetInterrupt (swfdec_js_runtime, swfdec_js_debug_one, NULL);
#endif
  JS_SetErrorReporter (s->jscx, swfdec_js_error_report);
  JS_SetContextPrivate(s->jscx, s);
  glob = JS_NewObject (s->jscx, &global_class, NULL, NULL);
  if (!JS_InitStandardClasses (s->jscx, glob)) {
    SWFDEC_ERROR ("initializing JS standard classes failed");
  }
  swfdec_js_add_globals (s, glob);
  swfdec_js_add_mouse (s, glob);
  swfdec_js_add_movieclip_class (s);
}

/**
 * swfdec_js_finish_decoder:
 * @s: a #SwfdecDecoder
 *
 * Shuts down the Javascript processing for @s.
 **/
void
swfdec_js_finish_decoder (SwfdecDecoder *s)
{
  //JSObject *global = JS_GetGlobalObject (s->jscx);

  if (s->jscx) {
    //JS_RemoveRoot (s->jscx, &global);
    JS_DestroyContext(s->jscx);
    s->jscx = NULL;
  }
}

/**
 * swfdec_js_execute_script:
 * @s: ia @SwfdecDecoder
 * @movie: a #SwfdecMovieClip to pass as argument to the script
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
swfdec_js_execute_script (SwfdecDecoder *s, SwfdecMovieClip *movie, 
    JSScript *script, jsval *rval)
{
  jsval returnval = JSVAL_VOID;
  JSBool ret;

  g_return_val_if_fail (s != NULL, FALSE);
  g_return_val_if_fail (SWFDEC_IS_MOVIE_CLIP (movie), FALSE);
  g_return_val_if_fail (script != NULL, FALSE);

#if 0
  g_print ("executing script %p:%p in frame %u\n", movie, script,
      movie->current_frame);
  swfdec_disassemble (s, script);
#endif
  if (rval == NULL)
    rval = &returnval;
  if (movie->jsobj == NULL) {
    swfdec_js_add_movieclip (movie);
    if (movie->jsobj == NULL)
      return FALSE;
  }
  ret = JS_ExecuteScript (s->jscx, movie->jsobj, script, rval);
  if (ret && returnval != JSVAL_VOID) {
    JSString * str = JS_ValueToString (s->jscx, returnval);
    if (str)
      g_print ("%s\n", JS_GetStringBytes (str));
  }
  return ret ? TRUE : FALSE;
}

/**
 * swfdec_js_run:
 * @dec: a #SwfdecDecoder
 * @s: JavaScript commands to execute
 * @rval: optional location to store a return value
 *
 * This is a debugging function for injecting script code into the @decoder.
 * Use it at your own risk.
 *
 * Returns: TRUE if the script was evaluated successfully. 
 **/
gboolean
swfdec_js_run (SwfdecDecoder *dec, const char *s, jsval *rval)
{
  gboolean ret;
  JSScript *script;
  JSObject *global;
  
  g_return_val_if_fail (dec != NULL, FALSE);
  g_return_val_if_fail (s != NULL, FALSE);

  global = JS_GetGlobalObject (dec->jscx);
  script = JS_CompileScript (dec->jscx, global, s, strlen (s), "injected-code", 1);
  if (script == NULL)
    return FALSE;
  ret = swfdec_js_execute_script (dec, dec->root, script, rval);
  JS_DestroyScript (dec->jscx, script);
  return ret;
}

/**
 * swfdec_value_to_string:
 * @dec: a #SwfdecDecoder
 * @val: a #jsval
 *
 * Converts the given jsval to its string representation.
 *
 * Returns: the string representation of @val.
 **/
const char *
swfdec_js_to_string (SwfdecDecoder *dec, jsval val)
{
  JSString *string;
  char *ret;

  g_return_val_if_fail (SWFDEC_IS_DECODER (dec), NULL);
  g_return_val_if_fail (dec->jscx != NULL, NULL);

  string = JS_ValueToString (dec->jscx, val);
  if (string == NULL || (ret = JS_GetStringBytes (string)) == NULL)
    return "[error]";

  return ret;
}
