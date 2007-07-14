/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_as_frame_internal.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_array.h"
#include "swfdec_as_context.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_super.h"
#include "swfdec_debug.h"

/**
 * SECTION:SwfdecAsFrame
 * @title: SwfdecAsFrame
 * @short_description: information about currently executing frames
 *
 * This section is only interesting for people that want to look into debugging.
 * A SwfdecAsFrame describes a currently executing function while it is
 * running. On every new function call, a new frame is created and pushed on top
 * of the frame stack. To get the topmost one, use 
 * swfdec_as_context_get_frame(). After that you can inspect various properties
 * of the frame, like the current stack.
 *
 * a #SwfdecAsFrame is a #SwfdecAsObject, so it is possible to set variables on
 * it. These are local variables inside the executing function. So you can use
 * functions such as swfdec_as_object_get_variable() to inspect them.
 */

/**
 * SwfdecAsFrame:
 *
 * the object used to represent an executing function.
 */

/*** STACK ITERATOR ***/

/**
 * SwfdecAsStackIterator:
 *
 * This is a struct used to walk the stack of a frame. It is supposed to be 
 * allocated on the stack. All of its members are private.
 */

SwfdecAsValue *
swfdec_as_stack_iterator_init_arguments (SwfdecAsStackIterator *iter, SwfdecAsFrame *frame)
{
  SwfdecAsContext *context;

  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (SWFDEC_IS_AS_FRAME (frame), NULL);
  /* FIXME! */
  context = SWFDEC_AS_OBJECT (frame)->context;
  g_return_val_if_fail (context->frame == frame, NULL);

  if (frame->argv) {
    iter->stack = NULL;
    iter->current = (SwfdecAsValue *) frame->argv;
  } else {
    iter->stack = context->stack;
    iter->current = frame->stack_begin - 1;
  }
  iter->i = 0;
  iter->n = frame->argc;
  if (frame->argc == 0)
    iter->current = NULL;
  return iter->current;
}

/**
 * swfdec_as_stack_iterator_init:
 * @iter: a #SwfdecStackIterator
 * @frame: the frame to initialize from
 *
 * Initializes @iter to walk the stack of @frame. The first value on the stack
 * will alread be returned. This makes it possible to write a simple loop to 
 * print the whole stack:
 * |[for (value = swfdec_as_stack_iterator_init (&amp;iter, frame); value != NULL;
 *     value = swfdec_as_stack_iterator_next (&amp;iter)) {
 *   char *s = swfdec_as_value_to_debug (value);
 *   g_print ("%s\n", s);
 *   g_free (s);
 * }]|
 *
 * Returns: the topmost value on the stack of @frame or %NULL if none
 **/
SwfdecAsValue *
swfdec_as_stack_iterator_init (SwfdecAsStackIterator *iter, SwfdecAsFrame *frame)
{
  SwfdecAsContext *context;
  SwfdecAsStack *stack;

  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (SWFDEC_IS_AS_FRAME (frame), NULL);

  context = SWFDEC_AS_OBJECT (frame)->context;
  iter->i = 0;
  stack = context->stack;
  if (context->frame == frame) {
    iter->current = context->cur;
  } else {
    SwfdecAsFrame *follow = context->frame;
    while (follow->next != frame)
      follow = follow->next;
    iter->current = follow->stack_begin;
    /* FIXME: get rid of arguments on stack */
    while (iter->current < &stack->elements[0] || iter->current > &stack->elements[stack->n_elements]) {
      stack = stack->next;
      g_assert (stack);
    }
  }
  iter->stack = stack;
  /* figure out number of args */
  iter->n = iter->current - &stack->elements[0];
  while (frame->stack_begin < &stack->elements[0] && frame->stack_begin > &stack->elements[stack->n_elements]) {
    iter->n += stack->used_elements;
    stack = stack->next;
  };
  g_assert (iter->n >= (guint) (frame->stack_begin - &stack->elements[0]));
  iter->n -= frame->stack_begin - &stack->elements[0];
  if (iter->n == 0)
    return NULL;
  if (iter->current == &iter->stack->elements[0]) {
    iter->stack = iter->stack->next;
    g_assert (iter->stack);
    iter->current = &iter->stack->elements[iter->stack->used_elements];
  }
  iter->current--;
  return iter->current;
}

/**
 * swfdec_as_stack_iterator_next:
 * @iter: a #SwfdecAsStackIterator
 *
 * Gets the next value on the stack.
 *
 * Returns: The next value on the stack or %NULL if no more values are on the stack
 **/
SwfdecAsValue *
swfdec_as_stack_iterator_next (SwfdecAsStackIterator *iter)
{
  if (iter->i < iter->n)
    iter->i++;
  if (iter->i >= iter->n)
    return NULL;
  if (iter->stack) {
    if (iter->current == &iter->stack->elements[0]) {
      iter->stack = iter->stack->next;
      g_assert (iter->stack);
      iter->current = &iter->stack->elements[iter->stack->used_elements];
    }
    iter->current--;
  } else {
    iter->current++;
  }
  return iter->current;
}


G_DEFINE_TYPE (SwfdecAsFrame, swfdec_as_frame, SWFDEC_TYPE_AS_SCOPE)

static void
swfdec_as_frame_dispose (GObject *object)
{
  SwfdecAsFrame *frame = SWFDEC_AS_FRAME (object);

  g_slice_free1 (sizeof (SwfdecAsValue) * frame->n_registers, frame->registers);
  if (frame->script) {
    swfdec_script_unref (frame->script);
    frame->script = NULL;
  }
  if (frame->constant_pool) {
    swfdec_constant_pool_free (frame->constant_pool);
    frame->constant_pool = NULL;
  }
  if (frame->constant_pool_buffer) {
    swfdec_buffer_unref (frame->constant_pool_buffer);
    frame->constant_pool_buffer = NULL;
  }

  G_OBJECT_CLASS (swfdec_as_frame_parent_class)->dispose (object);
}

static void
swfdec_as_frame_mark (SwfdecAsObject *object)
{
  SwfdecAsFrame *frame = SWFDEC_AS_FRAME (object);
  guint i;

  if (frame->next)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (frame->next));
  if (frame->scope)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (frame->scope));
  if (frame->thisp)
    swfdec_as_object_mark (frame->thisp);
  if (frame->super)
    swfdec_as_object_mark (frame->super);
  swfdec_as_object_mark (frame->target);
  swfdec_as_object_mark (frame->original_target);
  if (frame->function)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (frame->function));
  for (i = 0; i < frame->n_registers; i++) {
    swfdec_as_value_mark (&frame->registers[i]);
  }
  /* don't mark argv, it's const, others have to take care of it */
  SWFDEC_AS_OBJECT_CLASS (swfdec_as_frame_parent_class)->mark (object);
}

static char *
swfdec_as_frame_debug (SwfdecAsObject *object)
{
  SwfdecAsFrame *frame = SWFDEC_AS_FRAME (object);
  GString *string = g_string_new ("");
  SwfdecAsStackIterator iter;
  SwfdecAsValue *val;
  char *s;
  guint i;

  if (frame->thisp) {
    s = swfdec_as_object_get_debug (frame->thisp);
    g_string_append (string, s);
    g_string_append (string, ".");
    g_free (s);
  }
  g_string_append (string, frame->function_name);
  g_string_append (string, " (");
  i = 0;
  for (val = swfdec_as_stack_iterator_init_arguments (&iter, frame); val && i < 4;
      val = swfdec_as_stack_iterator_next (&iter)) {
    if (i > 0)
      g_string_append (string, ", ");
    i++;
    if (i == 3 && frame->argc > 4) {
      g_string_append (string, "...");
    } else {
      s = swfdec_as_value_to_debug (val);
      g_string_append (string, s);
      g_free (s);
    }
  }
  g_string_append (string, ")");
  return g_string_free (string, FALSE);
}

static void
swfdec_as_frame_class_init (SwfdecAsFrameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_frame_dispose;

  asobject_class->mark = swfdec_as_frame_mark;
  asobject_class->debug = swfdec_as_frame_debug;
}

static void
swfdec_as_frame_init (SwfdecAsFrame *frame)
{
  frame->function_name = "unnamed";
}

static void
swfdec_as_frame_load (SwfdecAsFrame *frame)
{
  SwfdecAsContext *context = SWFDEC_AS_OBJECT (frame)->context;

  frame->stack_begin = context->cur;
  context->base = frame->stack_begin;
  frame->next = context->frame;
  context->frame = frame;
  context->call_depth++;
}

SwfdecAsFrame *
swfdec_as_frame_new (SwfdecAsContext *context, SwfdecScript *script)
{
  SwfdecAsFrame *frame;
  gsize size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (script != NULL, NULL);
  
  size = sizeof (SwfdecAsFrame) + sizeof (SwfdecAsValue) * script->n_registers;
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  frame = g_object_new (SWFDEC_TYPE_AS_FRAME, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (frame), context, size);
  frame->script = swfdec_script_ref (script);
  frame->function_name = script->name;
  SWFDEC_DEBUG ("new frame for function %s", frame->function_name);
  frame->pc = script->buffer->data;
  frame->scope = SWFDEC_AS_SCOPE (frame);
  frame->n_registers = script->n_registers;
  frame->registers = g_slice_alloc0 (sizeof (SwfdecAsValue) * frame->n_registers);
  if (script->constant_pool) {
    frame->constant_pool_buffer = swfdec_buffer_ref (script->constant_pool);
    frame->constant_pool = swfdec_constant_pool_new_from_action (
	script->constant_pool->data, script->constant_pool->length, script->version);
    if (frame->constant_pool) {
      swfdec_constant_pool_attach_to_context (frame->constant_pool, context);
    } else {
      SWFDEC_ERROR ("couldn't create constant pool");
    }
  }
  swfdec_as_frame_load (frame);
  return frame;
}

SwfdecAsFrame *
swfdec_as_frame_new_native (SwfdecAsContext *context)
{
  SwfdecAsFrame *frame;
  gsize size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  
  size = sizeof (SwfdecAsFrame);
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  frame = g_object_new (SWFDEC_TYPE_AS_FRAME, NULL);
  SWFDEC_DEBUG ("new native frame");
  swfdec_as_object_add (SWFDEC_AS_OBJECT (frame), context, size);
  swfdec_as_frame_load (frame);
  return frame;
}

/**
 * swfdec_as_frame_return:
 * @frame: a #SwfdecAsFrame that is currently executing.
 * @return_value: return value of the function or %NULL for none. An undefined
 *                value will be used in that case.
 *
 * Ends execution of the frame and instructs the frame's context to continue 
 * execution with its parent frame. This function may only be called on the
 * currently executing frame.
 **/
void
swfdec_as_frame_return (SwfdecAsFrame *frame, SwfdecAsValue *return_value)
{
  SwfdecAsContext *context;
  SwfdecAsValue retval;
  SwfdecAsFrame *next;

  g_return_if_fail (SWFDEC_IS_AS_FRAME (frame));
  context = SWFDEC_AS_OBJECT (frame)->context;
  g_return_if_fail (frame == context->frame);

  /* save return value in case it was on the stack somewhere */
  if (frame->construct) {
    SWFDEC_AS_VALUE_SET_OBJECT (&retval, frame->thisp);
  } else if (return_value) {
    retval = *return_value;
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (&retval);
  }
  /* pop frame and leftover stack */
  next = frame->next;
  context->frame = next;
  g_assert (context->call_depth > 0);
  context->call_depth--;
  while (context->base > frame->stack_begin || 
      context->end < frame->stack_begin)
    swfdec_as_stack_pop_segment (context);
  context->cur = frame->stack_begin;
  /* setup stack for previous frame */
  if (next) {
    if (next->stack_begin >= &context->stack->elements[0] &&
	next->stack_begin <= context->cur) {
      context->base = next->stack_begin;
    } else {
      context->base = &context->stack->elements[0];
    }
  } else {
    g_assert (context->stack->next == NULL);
    context->base = &context->stack->elements[0];
  }
  /* pop argv if on stack */
  if (frame->argv == NULL && frame->argc > 0) {
    guint i = frame->argc;
    while (TRUE) {
      guint n = context->cur - context->base;
      n = MIN (n, i);
      swfdec_as_stack_pop_n (context, n);
      i -= n;
      if (i == 0)
	break;
      swfdec_as_stack_pop_segment (context);
    }
  }
  /* set return value */
  if (frame->return_value) {
    *frame->return_value = retval;
  } else {
    swfdec_as_stack_ensure_free (context, 1);
    *swfdec_as_stack_push (context) = retval;
  }
}

/**
 * swfdec_as_frame_set_this:
 * @frame: a #SwfdecAsFrame
 * @thisp: object to use as the this object
 *
 * Sets the object to be used as this pointer. If this function is not called,
 * the this value will be undefined.
 * You may only call this function once per @frame and it must be called 
 * directly after creating the frame and before calling swfdec_as_frame_preload().
 **/
void
swfdec_as_frame_set_this (SwfdecAsFrame *frame, SwfdecAsObject *thisp)
{
  g_return_if_fail (SWFDEC_IS_AS_FRAME (frame));
  g_return_if_fail (frame->thisp == NULL);
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (thisp));

  frame->thisp = thisp;
  if (frame->target == NULL) {
    frame->target = thisp;
    frame->original_target = thisp;
  }
}

/**
 * swfdec_as_frame_find_variable:
 * @frame: a #SwfdecAsFrame
 * @variable: name of the variable to find
 *
 * Finds the given variable in the current scope chain. Returns the first 
 * object in the scope chain that contains this variable in its prototype 
 * chain. If you want to know the explicit object that contains the variable,
 * you have to call swfdec_as_object_get_variable_and_flags() on the result.
 * If no such variable exist in the scope chain, %NULL is returned.
 * <note>The returned object might be an internal object. You probably do not
 * want to expose it to scripts. Call swfdec_as_object_resolve () on the
 * returned value to be sure of not having an internal object.</note>
 *
 * Returns: the object that contains @variable or %NULL if none.
 **/
SwfdecAsObject *
swfdec_as_frame_find_variable (SwfdecAsFrame *frame, const char *variable)
{
  SwfdecAsScope *cur;
  guint i;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_FRAME (frame), NULL);
  g_return_val_if_fail (variable != NULL, NULL);

  cur = frame->scope;
  for (i = 0; i < 256; i++) {
    if (swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (cur), variable, &val))
      return SWFDEC_AS_OBJECT (cur);
    if (cur->next == NULL)
      break;
    cur = cur->next;
  }
  if (i == 256) {
    swfdec_as_context_abort (SWFDEC_AS_OBJECT (frame)->context, "Scope recursion limit exceeded");
    return NULL;
  }
  g_assert (SWFDEC_IS_AS_FRAME (cur));
  /* we've walked the scope chain down. Now look in the special objects. */
  /* 1) the target */
  if (swfdec_as_object_get_variable (frame->target, variable, &val))
    return frame->target;
  /* 2) the global object */
  if (swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (frame)->context->global, variable, &val))
    return SWFDEC_AS_OBJECT (frame)->context->global;

  return NULL;
}

/* FIXME: merge with find_variable somehow */
gboolean
swfdec_as_frame_delete_variable (SwfdecAsFrame *frame, const char *variable)
{
  SwfdecAsScope *cur;
  guint i;

  g_return_val_if_fail (SWFDEC_IS_AS_FRAME (frame), FALSE);
  g_return_val_if_fail (variable != NULL, FALSE);

  cur = frame->scope;
  for (i = 0; i < 256; i++) {
    if (swfdec_as_object_delete_variable (SWFDEC_AS_OBJECT (cur), variable))
      return TRUE;
    if (cur->next == NULL)
      break;
    cur = cur->next;
  }
  if (i == 256) {
    swfdec_as_context_abort (SWFDEC_AS_OBJECT (frame)->context, "Scope recursion limit exceeded");
    return FALSE;
  }
  g_assert (SWFDEC_IS_AS_FRAME (cur));
  /* we've walked the scope chain down. Now look in the special objects. */
  /* 1) the target set via SetTarget */
  if (swfdec_as_object_delete_variable (frame->target, variable))
    return TRUE;
  /* 2) the global object */
  return swfdec_as_object_delete_variable (SWFDEC_AS_OBJECT (frame)->context->global, variable);
}

/**
 * swfdec_as_frame_set_target:
 * @frame: a #SwfdecAsFrame
 * @target: the new object to use as target or %NULL to unset
 *
 * Sets the new target to be used in this @frame. The target is a legacy 
 * Actionscript concept that is similar to "with". If you don't have to,
 * you shouldn't use this function.
 **/
void
swfdec_as_frame_set_target (SwfdecAsFrame *frame, SwfdecAsObject *target)
{
  g_return_if_fail (SWFDEC_IS_AS_FRAME (frame));
  g_return_if_fail (target == NULL || SWFDEC_IS_AS_OBJECT (target));

  if (target) {
    frame->target = target;
  } else {
    frame->target = frame->original_target;
  }
}

void
swfdec_as_frame_preload (SwfdecAsFrame *frame)
{
  SwfdecAsObject *object;
  guint i, current_reg = 1;
  SwfdecScript *script;
  SwfdecAsStack *stack;
  SwfdecAsValue val;
  const SwfdecAsValue *cur;
  SwfdecAsContext *context;

  g_return_if_fail (SWFDEC_IS_AS_FRAME (frame));

  if (frame->script == NULL)
    return;

  object = SWFDEC_AS_OBJECT (frame);
  context = object->context;
  script = frame->script;
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_THIS) {
    if (frame->thisp) {
      SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], frame->thisp);
    } else {
      current_reg++;
    }
  } else if (!(script->flags & SWFDEC_SCRIPT_SUPPRESS_THIS)) {
    if (frame->thisp) {
      SWFDEC_AS_VALUE_SET_OBJECT (&val, frame->thisp);
    } else {
      SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
    }
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_this, &val);
  }
  if (!(script->flags & SWFDEC_SCRIPT_SUPPRESS_ARGS)) {
    SwfdecAsObject *args = swfdec_as_array_new (context);

    if (!args)
      return;
    if (frame->argc > 0) {
      if (frame->argv) {
	swfdec_as_array_append (SWFDEC_AS_ARRAY (args), frame->argc, frame->argv);
      } else {
	stack = context->stack;
	cur = context->cur;
	for (i = 0; i < frame->argc; i++) {
	  if (cur <= &stack->elements[0]) {
	    stack = stack->next;
	    cur = &stack->elements[stack->used_elements];
	  }
	  cur--;
	  swfdec_as_array_push (SWFDEC_AS_ARRAY (args), cur);
	}
      }
    }
    /* FIXME: implement callee/caller */
    if (script->flags & SWFDEC_SCRIPT_PRELOAD_ARGS) {
      SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], args);
    } else {
      SWFDEC_AS_VALUE_SET_OBJECT (&val, args);
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_arguments, &val);
    }
  }
  if (!(script->flags & SWFDEC_SCRIPT_SUPPRESS_SUPER)) {
    frame->super = swfdec_as_super_new (frame);
    if (frame->super) {
      if (script->flags & SWFDEC_SCRIPT_PRELOAD_SUPER) {
	SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], frame->super);
      } else {
	SWFDEC_AS_VALUE_SET_OBJECT (&val, frame->super);
	swfdec_as_object_set_variable (object, SWFDEC_AS_STR_super, &val);
      }
    }
  }
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_ROOT) {
    SwfdecAsObject *obj;
    
    obj = swfdec_as_frame_find_variable (frame, SWFDEC_AS_STR__root);
    if (obj) {
      swfdec_as_object_get_variable (obj, SWFDEC_AS_STR__root, &frame->registers[current_reg]);
    } else {
      SWFDEC_WARNING ("no root to preload");
    }
    current_reg++;
  }
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_PARENT) {
    SwfdecAsObject *obj;
    
    obj = swfdec_as_frame_find_variable (frame, SWFDEC_AS_STR__parent);
    if (obj) {
      swfdec_as_object_get_variable (obj, SWFDEC_AS_STR__parent, &frame->registers[current_reg++]);
    } else {
      SWFDEC_WARNING ("no parent to preload");
    }
    current_reg++;
  }
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_GLOBAL) {
    SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], context->global);
  }
  stack = context->stack;
  SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  cur = frame->argv ? frame->argv - 1 : context->cur;
  for (i = 0; i < script->n_arguments; i++) {
    /* first figure out the right value to set */
    if (i >= frame->argc) {
      cur = &val;
    } else if (frame->argv) {
      cur++;
    } else {
      if (cur <= &stack->elements[0]) {
	stack = stack->next;
	cur = &stack->elements[stack->used_elements];
      }
      cur--;
    }
    /* now set this value at the right place */
    if (script->arguments[i].preload) {
      if (script->arguments[i].preload < frame->n_registers) {
	frame->registers[script->arguments[i].preload] = *cur;
      } else {
	SWFDEC_ERROR ("trying to set %uth argument %s in nonexisting register %u", 
	    i, script->arguments[i].name, script->arguments[i].preload);
      }
    } else {
      const char *tmp = swfdec_as_context_get_string (context, script->arguments[i].name);
      swfdec_as_object_set_variable (object, tmp, cur);
    }
  }
}

/**
 * swfdec_as_frame_check_scope:
 * @frame: a #SwfdecAsFrame
 *
 * Checks that the current scope of the given @frame is still correct.
 * If it is not, the current scope is popped and the next one is used.
 * If the
 **/
void
swfdec_as_frame_check_scope (SwfdecAsFrame *frame)
{
  SwfdecAsScope *frame_scope;

  g_return_if_fail (SWFDEC_IS_AS_FRAME (frame));

  frame_scope = SWFDEC_AS_SCOPE (frame);
  while (frame->scope != frame_scope) {
    SwfdecAsScope *cur = frame->scope;

    if (frame->pc >= cur->startpc && 
	frame->pc < cur->endpc)
      break;
    
    frame->scope = cur->next;
  }
}

/**
 * swfdec_as_frame_get_next:
 * @frame: a #SwfdecAsFrame
 *
 * Gets the next frame in the frame stack. The next frame is the frame that
 * will be executed after this @frame.
 *
 * Returns: the next #SwfdecAsFrame or %NULL if this is the bottommost frame.
 **/
SwfdecAsFrame *
swfdec_as_frame_get_next (SwfdecAsFrame *frame)
{
  g_return_val_if_fail (SWFDEC_IS_AS_FRAME (frame), NULL);

  return frame->next;
}

