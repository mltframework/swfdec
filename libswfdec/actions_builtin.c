#include <ctype.h>
#include <string.h>
#include <math.h>

#include "swfdec_internal.h"

struct mc_list_entry {
  JSObject *mc;
  SwfdecSpriteSegment *seg;
};

static JSBool
mc_play(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecSpriteSegment *seg;

  seg = JS_GetPrivate(cx, obj);
  if (!seg) {
    SWFDEC_WARNING("couldn't get segment");
    return JS_FALSE;
  }
  seg->stopped = FALSE;

  return JS_TRUE;
}

static JSBool
mc_stop(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecSpriteSegment *seg;

  seg = JS_GetPrivate(cx, obj);
  if (!seg) {
    SWFDEC_WARNING("couldn't get segment");
    return JS_FALSE;
  }
  seg->stopped = FALSE;

  return JS_TRUE;
}

static JSBool
mc_attachMovie(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
    jsval *rval)
{
  SwfdecSpriteSegment *seg;

  seg = JS_GetPrivate(cx, obj);
  if (!seg) {
    SWFDEC_WARNING("couldn't get segment");
    return JS_FALSE;
  }

  return JS_TRUE;
}

/* MovieClip AS standard class */

enum {
	MC_X = -1,
	MC_Y = -2,
};

#define MC_PROP_ATTRS (JSPROP_PERMANENT|JSPROP_SHARED)
static JSPropertySpec movieclip_props[] = {
	{"_x",	MC_X,	MC_PROP_ATTRS, 0, 0},
	{"_y",	MC_Y,	MC_PROP_ATTRS, 0, 0},
};

JSClass movieclip_class = {
    "MovieClip", JSCLASS_NEW_RESOLVE,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   JS_FinalizeStub
};

static JSFunctionSpec movieclip_methods[] = {
  {"play", mc_play, 0, 0},
  {"stop", mc_stop, 0, 0},
  {"attachMovie", mc_attachMovie, 4, 0},
  {0, 0, 0, 0, 0}
};

static JSObject *
movieclip_new (SwfdecActionContext *context, SwfdecSpriteSegment *seg)
{
  JSObject *mc;
  struct mc_list_entry *listentry;

  mc = JS_NewObject (context->jscx, &movieclip_class, NULL, NULL);
  JS_AddRoot (context->jscx, mc);
  JS_SetPrivate (context->jscx, mc, seg);

  listentry = g_malloc (sizeof (struct mc_list_entry));
  listentry->mc = mc;
  listentry->seg = seg;
  context->seglist = g_list_append (context->seglist, listentry);

  return mc;
}

JSObject *
movieclip_find (SwfdecActionContext *context,
    SwfdecSpriteSegment *seg)
{
  GList *g;
  struct mc_list_entry *listentry;

  for (g = g_list_first (context->seglist); g; g = g_list_next (g)) {
    listentry = (struct mc_list_entry *)g->data;
SWFDEC_WARNING("%p %p", seg, listentry->seg);
    if (listentry->seg == seg)
      return listentry->mc;
  }

  return NULL;
}

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
  JSObject *MovieClip, *root;
  JSBool ok;
  jsval val;

  context->global = JS_NewObject (context->jscx, &global_class, NULL, NULL);
  if (!context->global)
    return;
  if (!JS_InitStandardClasses (context->jscx, context->global))
    return;

  MovieClip = JS_InitClass (context->jscx, context->global, NULL,
      &movieclip_class, NULL, 0, movieclip_props, movieclip_methods,
      NULL, NULL);

  root = movieclip_new (context, context->s->main_sprite_seg);
  ok = JS_SetProperty(context->jscx, context->global, "_root", &val);
  if (!ok)
    SWFDEC_WARNING("Failed to set _root");
}

void
action_register_sprite_seg (SwfdecDecoder * s, SwfdecSpriteSegment *seg)
{
  SwfdecActionContext *context;
  JSObject *mc;
  JSBool ok;
  jsval val;

  SWFDEC_DEBUG ("Placing MovieClip %s", seg->name ? seg->name : "(no name)");

  if (s->context == NULL)
    swfdec_init_context (s);
  context = s->context;

  mc = movieclip_new (context, seg);
  val = OBJECT_TO_JSVAL(mc);

  if (seg->name) {
    ok = JS_SetProperty(context->jscx, context->global, seg->name, &val);
    if (!ok)
      SWFDEC_WARNING("Failed to register %s", seg->name);
  }
}
