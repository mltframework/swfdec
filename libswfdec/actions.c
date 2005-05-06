#include <ctype.h>
#include <string.h>

#include "swfdec_internal.h"
#include "js/jsfun.h"

void
constant_pool_ref (ConstantPool *pool)
{
  pool->refcount++;
}

void
constant_pool_unref (ConstantPool *pool)
{
  if (--pool->refcount == 0) {
    int i;

    for (i=0;i<pool->n_constants;i++){
      g_free(pool->constants[i]);
    }
    g_free (pool->constants);
    g_free (pool);
  }
}

int
tag_func_do_action (SwfdecDecoder * s)
{
  swfdec_sprite_add_action (s->parse_sprite, s->b.buffer,
      s->parse_sprite->parse_frame);

  s->b.ptr += s->b.buffer->length;

  return SWF_OK;
}

int
pc_is_valid (SwfdecActionContext *context, unsigned char *pc)
{
  unsigned char *startpc;
  unsigned char *endpc;

  startpc = context->bits.buffer->data;
  endpc = context->bits.buffer->data + context->bits.buffer->length;
  /* Use a <= here because sometimes you'll get a jump to endpc in functions as
   * a way of branching to return (without an ActionReturn).  action_script_call
   * will handle making the return happen.
   */
  if (pc >= startpc && pc <= endpc) {
    return SWF_OK;
  } else {
    return SWF_ERROR;
  }
}

void swfdec_init_context (SwfdecDecoder * s)
{
  SwfdecActionContext *context;
  int i;
  jsval val;

  context = g_malloc0 (sizeof(SwfdecActionContext));
  s->context = context;
  context->s = s;
  context->call_stack = g_queue_new();

  context->jsrt = JS_NewRuntime (64L * 1024L * 1024L);
  if (!context->jsrt)
    return;

  context->jscx = JS_NewContext (context->jsrt, 8192);

  JS_SetContextPrivate (context->jscx, context);

  context->stack = JS_NewArrayObject (context->jscx, 0, NULL);
  context->stack_top = 0;
  JS_AddRoot (context->jscx, context->stack);

  context->registers = JS_NewArrayObject (context->jscx, 0, NULL);
  JS_AddRoot (context->jscx, context->registers);
  for (i = 0; i < 4; i++) {
      val = JSVAL_VOID;
      JS_SetElement (context->jscx, context->registers, i, &val);
  }

  swfdec_init_context_builtins (context);
}

JSBool
action_script_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, 
	jsval *rval)
{
  int action;
  int len;
  ActionFuncEntry *func_entry;
  CallFrame *frame;
  SwfdecActionContext *context;
  JSFunction *jsfunc;
  ScriptFunction *func;

  /* argv[-2] is funobj, argc[-1] is this, argv[0] through argv[argc-1] are the
   * args, and beyond that are 3 tag temporaries and then the array object for
   * registers.
   */

  context = JS_GetContextPrivate (cx);
  jsfunc = JS_ValueToFunction (cx, argv[-2]);
  func = jsfunc->priv;

  /* FIXME: set limits on recursion to prevent stack exhaustion */

  /* Native methods short circuit this whole mess. */
  if (func->meth) {
    return func->meth (context, argc);
  }

  frame = g_malloc0 (sizeof(CallFrame));
  frame->returnpc = context->pc;
  frame->is_function2 = func->is_function2;
  frame->bits.buffer = func->buffer;
  frame->bits.ptr = func->pc;
  frame->bits.idx = 0;
  frame->bits.end = func->pc + func->code_size;
  frame->returnbuffer = context->bits.buffer;
  frame->returnpool = context->constantpool;
  frame->returnargv = context->tag_argv;
  frame->returnargc = context->tag_argc;
  context->constantpool = func->constantpool;
  if (func->constantpool)
    constant_pool_ref(func->constantpool);
  context->pc = func->pc;
  context->tag_argv = argv;
  context->tag_argc = argc;
  g_queue_push_head (context->call_stack, frame);
  context->call_depth++;

  if (func->is_function2) {
    int i;

    frame->registers = JS_NewArrayObject (context->jscx, 0, NULL);
    argv[argc + 3] = OBJECT_TO_JSVAL(frame->registers);

    i = 1;
    if (func->preload_this) {
      jsval val = OBJECT_TO_JSVAL(obj);
      JS_SetElement (context->jscx, frame->registers, i++, &val);
    }
    if (func->preload_super) {
      jsval val = OBJECT_TO_JSVAL(JS_GetConstructor (context->jscx,
        JS_GetParent (context->jscx, obj)));
      JS_SetElement (context->jscx, frame->registers, i++, &val);
    }
    if (func->preload_root) {
      jsval val = OBJECT_TO_JSVAL(movieclip_find(context,
          context->s->main_sprite_seg));
      JS_SetElement (context->jscx, frame->registers, i++, &val);
    }
    if (func->preload_parent) {
      jsval val = OBJECT_TO_JSVAL(JS_GetParent (context->jscx, obj));
      JS_SetElement (context->jscx, frame->registers, i++, &val);
    }
    if (func->preload_global) {
      jsval val = OBJECT_TO_JSVAL(context->global);
      JS_SetElement (context->jscx, frame->registers, i++, &val);
    }

    for (i = 0; (i < argc) && (i < func->num_params); i++) {
      if (func->param_regs[i]) {
        jsval val = stack_pop (context);
        JS_SetElement (context->jscx, frame->registers, func->param_regs[i],
            &val);
      } else {
        (void)stack_pop (context);
        SWFDEC_WARNING ("named arguments unsupported");
      }
    }
  }

  context->bits.buffer = frame->bits.buffer; /* for pc_is_valid () */

  while (context->pc < frame->bits.end) {
    action = swfdec_bits_get_u8 (&frame->bits);
    if (action & 0x80) {
      len = swfdec_bits_get_u16 (&frame->bits);
      context->bits.buffer = frame->bits.buffer;
      context->bits.ptr = frame->bits.ptr;
      context->bits.idx = 0;
      context->bits.end = frame->bits.ptr + len;
    } else {
      len = 0;
    }
    frame->bits.ptr += len;
    context->pc = frame->bits.ptr;
    context->action = action;

    func_entry = get_action (action);

    if (context->skip > 0) {
      SWFDEC_DEBUG ("[skip] depth %d, action 0x%02x (%s)", context->call_depth,
        action, func_entry ? func_entry->name : "(unknown)");
      context->skip--;
      continue;
    }

    SWFDEC_DEBUG ("depth %d, action 0x%02x (%s)", context->call_depth,
      action, func_entry ? func_entry->name : "(unknown)");
    if (func_entry) {
      func_entry->func (context);
    } else {
      SWFDEC_WARNING ("unknown action 0x%02x, ignoring", action);
      context->error = 1;
    }

    if (len) {
      if (context->bits.ptr < context->bits.end) {
        SWFDEC_ERROR ("action didn't read all data (%d < %d)",
            context->bits.ptr - (frame->bits.ptr - len),
            context->bits.end - (frame->bits.ptr - len));
      }
      if (context->bits.ptr > context->bits.end) {
        SWFDEC_ERROR ("action read past end of buffer (%d > %d)",
            context->bits.ptr - (frame->bits.ptr - len),
            context->bits.end - (frame->bits.ptr - len));
      }
    }

    if (frame->done)
      break;

    frame->bits.ptr = context->pc;

    if (context->error) {
      context->error = 0;
      SWFDEC_ERROR ("action script error");
    }
  }

  SWFDEC_DEBUG("returning from actionscript call");

  *rval = stack_pop (context);

  if (context->constantpool) {
    constant_pool_unref (context->constantpool);
  }
  context->constantpool = frame->returnpool;
  context->bits.buffer = frame->returnbuffer;
  context->pc = frame->returnpc;
  context->tag_argv = frame->returnargv;
  context->tag_argc = frame->returnargc;
  g_queue_pop_head (context->call_stack);
  context->call_depth--;
  g_free (frame);

  return 1;
}

int
swfdec_action_script_execute (SwfdecDecoder * s, SwfdecBuffer * buffer)
{
  SwfdecActionContext *context;
  jsval rval;
  JSFunction *jsfunc;
  ScriptFunction *func;
  JSObject *root;

  if (!s->parse_sprite) {
    s->parse_sprite = s->main_sprite;
    s->parse_sprite_seg = s->main_sprite_seg;
  }

  SWFDEC_INFO ("swfdec_action_script_execute (sprite %d) %p %p %d",
      SWFDEC_OBJECT(s->parse_sprite)->id, buffer, buffer->data, buffer->length);

  if (s->context == NULL)
    swfdec_init_context (s);
  context = s->context;

  func = g_malloc0 (sizeof (ScriptFunction));
  if (func == NULL) {
    SWFDEC_ERROR ("out of memory");
    return SWF_ERROR;
  }

  func->num_params = 0;
  func->code_size = buffer->length;
  func->pc = buffer->data;
  func->buffer = buffer;

  jsfunc = JS_NewFunction (context->jscx, action_script_call, func->num_params,
    0, context->global, func->name);
  jsfunc->priv = func;

  root = movieclip_find(context, context->s->main_sprite_seg);
  JS_CallFunction (context->jscx, root, jsfunc, 0, NULL, &rval);

  /* FIXME: as of Flash 6/7, stack gets emptied at the end of every action
   * block. -- flasm.sf.net
   */
  return SWF_OK;
}

int
tag_func_do_init_action (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int len;
  SwfdecBuffer *buffer;
  SwfdecObject *obj;
  SwfdecSprite *save_parse_sprite;
  unsigned char *endptr;
  int retcode = SWF_ERROR;

  endptr = bits->ptr + bits->buffer->length;

  obj = swfdec_object_get (s, swfdec_bits_get_u16 (bits));

  len = bits->end - bits->ptr;

  buffer = swfdec_buffer_new_subbuffer (bits->buffer,
    bits->ptr - bits->buffer->data, len);

  bits->ptr += len;

  if (SWFDEC_IS_SPRITE(obj)) {
    save_parse_sprite = s->parse_sprite;
    s->parse_sprite = SWFDEC_SPRITE(obj);
    retcode = swfdec_action_script_execute (s, buffer);
    s->parse_sprite = save_parse_sprite;
  }
  swfdec_buffer_unref (buffer);

  return retcode;
}

/* stack ops */

jsval
stack_pop (SwfdecActionContext *context)
{
  JSBool ok;
  jsval val;

  if (context->stack_top == 0)
    return JSVAL_VOID;

  ok = JS_GetElement (context->jscx, context->stack, --context->stack_top,
    &val);

  if (!ok)
    SWFDEC_WARNING("Couldn't pop element");

  return val;
}

void
stack_push (SwfdecActionContext *context, jsval val)
{
  JSBool ok;

  ok = JS_SetElement (context->jscx, context->stack, context->stack_top++,
    &val);

  if (!ok)
    SWFDEC_WARNING("Couldn't push element");
}

static char *
name_object (SwfdecActionContext *context, JSObject *obj)
{
  /* FIXME: I envision this function walking the tree from _global, trying to
   * find a name for the object that's on the stack.  That might be
   * unreasonable.  For now, identify if the object is _global.
   */

  if (obj == context->global) {
    return g_strdup ("_global");
  } else {
    char *name = g_malloc (20);
    g_snprintf(name, 20, "%p", obj);
    return name;
  }
}

void
stack_show (SwfdecActionContext *context)
{
  jsval val;
  int i = 0;

  SWFDEC_DEBUG ("Stack:");
  for (i = context->stack_top - 1; i >= 0; i--) {
    JS_GetElement (context->jscx, context->stack, i, &val);
    if (JSVAL_IS_NULL(val)) {
      SWFDEC_DEBUG (" %d: (null)", i);
    } else if (JSVAL_IS_VOID(val)) {
      SWFDEC_DEBUG (" %d: (undefined)", i);
    } else if (JSVAL_IS_STRING(val)) {
      SWFDEC_DEBUG (" %d: \"%s\"", i, JS_GetStringBytes(JSVAL_TO_STRING (val)));
    } else if (JSVAL_IS_INT(val)) {
      SWFDEC_DEBUG (" %d: %f", i, JSVAL_TO_INT (val));
    } else if (JSVAL_IS_DOUBLE(val)) {
      SWFDEC_DEBUG (" %d: %f", i, *JSVAL_TO_DOUBLE (val));
    } else if (JSVAL_IS_BOOLEAN(val)) {
      SWFDEC_DEBUG (" %d: %s", i, JSVAL_TO_BOOLEAN (val) ? "true" : "false");
    } else if (JSVAL_IS_OBJECT(val)) {
      char *name = name_object (context, JSVAL_TO_OBJECT(val));
      SWFDEC_DEBUG (" %d: obj (%s)", i, name);
      g_free (name);
    } else {
      SWFDEC_DEBUG (" %d: unknown type", i);
    }
  }
}

/* Given a jsval, convert it into an object.  JS_ValueToObject appears to have
 * some side effect when working on an existing object, at least.  While I'm
 * here, make it print out when we actually convert a non-object to object.
 * Perhaps this never occurs in actionscript bytecode.
 */
JSObject *
jsval_as_object (SwfdecActionContext *context, jsval val)
{
  if (JSVAL_IS_OBJECT(val)) {
    return JSVAL_TO_OBJECT(val);
  } else {
    JSObject *obj = NULL;
    JSBool ok;

    SWFDEC_INFO("Converting value %x to object", val);
    ok = JS_ValueToObject (context->jscx, val, &obj);
    if (!ok) {
      SWFDEC_ERROR("Couldn't convert value %x to object", val);
    }
    return obj;
  }
}
