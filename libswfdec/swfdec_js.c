
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <js/jsapi.h>
#include "swfdec_types.h"
#include "swfdec_decoder.h"
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_compiler.h"

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

static JSClass global_class = {
  "global",0,
  JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
  JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub
};

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

  JS_SetContextPrivate(s->jscx, s);
  glob = JS_NewObject (s->jscx, &global_class, NULL, NULL);
  if (!JS_InitStandardClasses (s->jscx, glob)) {
    SWFDEC_ERROR ("initializing JS standard classes failed");
  }
  swfdec_js_add_movieclip (s);
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
  JSObject *global = JS_GetGlobalObject (s->jscx);

  JS_RemoveRoot (s->jscx, global);
  if (s->jscx) {
    JS_DestroyContext(s->jscx);
    s->jscx = NULL;
  }
}

/**
 * swfdec_decoder_queue_script:
 * @s: a #SwfdecDecoder
 * @script: script to queue for execution
 *
 * Queues a script for execution at the next execution point.
 **/
void
swfdec_decoder_queue_script (SwfdecDecoder *s, JSScript *script)
{
  SWFDEC_DEBUG ("adding script %p to list", script);
  s->execute_list = g_list_prepend (s->execute_list, script);
}

/**
 * swfdec_decoder_execute_scripts:
 * @s: a #SwfdecDecoder
 *
 * Executes all queued up scripts in the order they were queued.
 **/
void
swfdec_decoder_execute_scripts (SwfdecDecoder *s)
{
  GList *walk;

  s->execute_list = g_list_reverse (s->execute_list);
  for (walk = s->execute_list; walk; walk = walk->next) {
    swfdec_js_execute_script (s, walk->data);
  }
  g_list_free (s->execute_list);
  s->execute_list = NULL;
}

/**
 * swfdec_js_execute_script:
 * @s: ia @SwfdecDecoder
 * @script: a @JSScript to execute
 *
 * Executes the given @script in the given @decoder. This function is supposed
 * to be the single entry point for running JavaScript code inswide swfdec, so
 * if you want to execute code, please use this function.
 *
 * Returns: TRUE if the script was successfully executed
 **/
gboolean
swfdec_js_execute_script (SwfdecDecoder *s, JSScript *script)
{
  jsval rval;
  JSBool ret;
  JSObject *global;

  g_return_val_if_fail (s != NULL, FALSE);
  g_return_val_if_fail (script != NULL, FALSE);

  //swfdec_disassemble (s, script);
  global = JS_GetGlobalObject (s->jscx);
  ret = JS_ExecuteScript (s->jscx, global, script, &rval);
  if (ret && rval != JSVAL_VOID) {
    JSString * str = JS_ValueToString (s->jscx, rval);
    if (str)
      g_print ("%s\n", JS_GetStringBytes (str));
  }
  return ret ? TRUE : FALSE;
}

/**
 * swfdec_js_run:
 * @dec: a #SwfdecDecoder
 * @s: JavaScript commands to execute
 *
 * This is a debugging function for injecting script code into the @decoder.
 * Use it at your own risk.
 *
 * Returns: TRUE if the script was evaluated successfully. 
 **/
gboolean
swfdec_js_run (SwfdecDecoder *dec, const char *s)
{
  gboolean ret;
  JSScript *script;
  JSObject *global;
  
  g_return_val_if_fail (dec != NULL, FALSE);
  g_return_val_if_fail (s != NULL, FALSE);

  global = JS_GetGlobalObject (dec->jscx);
  script = JS_CompileScript (dec->jscx, global, s, strlen (s), "injected-code", 1);
  ret = swfdec_js_execute_script (dec, script);
  JS_DestroyScript (dec->jscx, script);
  return ret;
}

