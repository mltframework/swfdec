#include <ctype.h>
#include <string.h>
#include <math.h>

#include "swfdec_internal.h"

/*ScriptObject *
obj_new_Object ()
{
  ScriptObject *obj;

  obj = g_malloc0(sizeof(ScriptObject));
  obj_ref (obj);

  return obj;
}

ScriptObject *
obj_new_Function ()
{
  ScriptObject *obj, *obj2;
  ActionVal *a;

  obj = g_malloc0(sizeof(ScriptObject));
  obj_ref (obj);

  obj2 = obj_new_Object ();
  a = action_val_new ();
  action_val_set_to_object(a, obj);
  obj_set_property (obj2, "constructor", a, OBJPROP_DONTENUM);
  action_val_free (a);

  a = action_val_new ();
  action_val_set_to_object(a, obj2);
  obj_set_property (obj, "prototype", a, OBJPROP_DONTDELETE);
  action_val_free (a);

  return obj;
}

ScriptObject *
obj_new_MovieClip ()
{
  ScriptObject *obj;

  obj = obj_new_Object();

  return obj;
}
*/

#if 0
static void
swfdec_native_ASSetPropFlags (SwfdecActionContext *context, int num_args,
  ActionVal *_this)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;
  ActionVal *d;
  int allowFalse = 0;
  int flags;

  a = stack_pop (context); /* obj */
  action_val_convert_to_object (a);
  b = stack_pop (context); /* property list */
  c = stack_pop (context); /* flags */
  action_val_convert_to_number (c);
  if (num_args >= 4) {
    d = stack_pop (context); /* allowFalse */
    action_val_convert_to_boolean (d);
    allowFalse = d->number;
    action_val_free (d);
  }

  flags = (int)c->number & 0x7;
  /* The flags appear to be 0x1 for DontEnum, 0x2 for DontDelete, and 0x4 for
   * DontWrite, though the tables I found on the web are poorly written.
   */

  if (ACTIONVAL_IS_NULL(b)) {
    GList *g;

    SWFDEC_DEBUG("%d args", num_args);

    for (g = g_list_first (a->obj->properties); g; g = g_list_next (g)) {
      ScriptObjectProperty *prop = g->data;
      if (allowFalse) {
        prop->flags = flags;
      } else {
        prop->flags |= flags;
      }
    }
  } else {
    action_val_convert_to_string (b);
    SWFDEC_WARNING("ASSetPropFlags not implemented (properties %s, flags 0x%x)",
      b->string, (int)c->number);
  }

  action_val_free (a);
  action_val_free (b);
  action_val_free (c);

  a = action_val_new ();
  a->type = ACTIONVAL_TYPE_UNDEF;
  stack_push (context, a);
}
#endif

JSClass global_class = {
    "global", JSCLASS_NEW_RESOLVE,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   JS_FinalizeStub
};

void swfdec_init_context_builtins (SwfdecActionContext *context)
{ 
  context->global = JS_NewObject(context->jscx, &global_class, NULL, NULL);
  if (!context->global)
    return;
  if (!JS_InitStandardClasses(context->jscx, context->global))
    return;
  /*if (!JS_DefineFunctions(context->jscx, context->global, shell_functions))
    return;*/

  /* _global.ASSetPropFlags */
  /*add_native_method_property (_global, "ASSetPropFlags", 4,
    swfdec_native_ASSetPropFlags);*/
}

JSClass movieclip_class = {
    "MovieClip", JSCLASS_NEW_RESOLVE,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   JS_FinalizeStub
};

void
action_register_sprite_name (SwfdecDecoder * s, char *name, int id)
{
  SwfdecActionContext *context;
  JSObject *obj;
  jsval val;
  JSBool ok;

  SWFDEC_DEBUG ("registering sprite %s", name);

  if (s->context == NULL)
    swfdec_init_context (s);
  context = s->context;

  obj = JS_NewObject(context->jscx, &movieclip_class, NULL, NULL);
  val = OBJECT_TO_JSVAL(obj);
  ok = JS_SetProperty(context->jscx, context->global, name, &val);
  if (!ok)
    SWFDEC_WARNING("Failed to register %s", name);

  val = INT_TO_JSVAL(id);
  JS_SetProperty(context->jscx, obj, "__swfdec_id", &val);
}
