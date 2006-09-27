#include "swfdec_internal.h"
#include "swfdec_js.h"

static JSBool
swfdec_js_mouse_show (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecDecoder *s = JS_GetContextPrivate (cx);

  g_assert (s);
  if (!s->mouse_visible) {
    s->mouse_visible = TRUE;
    g_object_notify (G_OBJECT (s), "mouse-visible");
  }
  return JS_TRUE;
}

static JSBool
swfdec_js_mouse_hide (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecDecoder *s = JS_GetContextPrivate (cx);

  g_assert (s);
  if (s->mouse_visible) {
    s->mouse_visible = FALSE;
    g_object_notify (G_OBJECT (s), "mouse-visible");
  }
  return JS_TRUE;
}

static JSFunctionSpec mouse_methods[] = {
    {"show",		swfdec_js_mouse_show,	0, 0, 0 },
    {"hide",		swfdec_js_mouse_hide,	0, 0, 0 },
    {0,0,0,0,0}
};

static JSClass mouse_class = {
    "Mouse",
    0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

void
swfdec_js_add_mouse (SwfdecDecoder *dec, JSObject *global)
{
  JSObject *mouse;
  
  mouse = JS_DefineObject(dec->jscx, global, "Mouse", &mouse_class, NULL, 0);
  if (!mouse || 
      !JS_DefineFunctions(dec->jscx, mouse, mouse_methods)) {
    SWFDEC_ERROR ("failed to initialize Mouse object");
  }
}

