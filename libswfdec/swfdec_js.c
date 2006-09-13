
#ifndef __SWFDEC_JS_H__
#define __SWFDEC_JS_H__

#include <js/jsapi.h>
#include "swfdec_types.h"
#include "swfdec_decoder.h"
#include "swfdec_debug.h"

G_BEGIN_DECLS

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

  glob = JS_NewObject (s->jscx, &global_class, NULL, NULL);
  if (!JS_InitStandardClasses (s->jscx, glob)) {
    SWFDEC_ERROR ("initializing JS standard classes failed");
  }
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

G_END_DECLS

#endif
