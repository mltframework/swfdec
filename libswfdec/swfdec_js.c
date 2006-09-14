
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <js/jsapi.h>
#include "swfdec_types.h"
#include "swfdec_decoder.h"
#include "swfdec_debug.h"
#include "swfdec_js.h"

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
    /* lalala */
  }
  g_list_free (s->execute_list);
  s->execute_list = NULL;
}

