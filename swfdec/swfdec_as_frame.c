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
#include "swfdec_movie.h"

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

/**
 * swfdec_as_stack_iterator_init_arguments:
 * @iter: iterator to be initialized
 * @frame: the frame to initialize from
 *
 * Initializes a stack iterator to walk the arguments passed to the given @frame. See
 * swfdec_as_stack_iterator_init() about suggested iterator usage.
 *
 * Returns: The value of the first argument
 **/
SwfdecAsValue *
swfdec_as_stack_iterator_init_arguments (SwfdecAsStackIterator *iter, SwfdecAsFrame *frame)
{
  SwfdecAsContext *context;

  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (frame != NULL, NULL);

  if (frame->argc == 0) {
    iter->i = iter->n = 0;
    iter->stack = NULL;
    iter->current = NULL;
    return NULL;
  }
  context = swfdec_gc_object_get_context (frame->target);
  if (frame->argv) {
    iter->stack = NULL;
    iter->current = (SwfdecAsValue *) frame->argv;
  } else {
    SwfdecAsStack *stack = context->stack;
    SwfdecAsValue *end;
    iter->current = frame->stack_begin - 1;
    end = context->cur;
    while (iter->current < stack->elements ||
	iter->current > end) {
      stack = stack->next;
      end = &stack->elements[stack->used_elements];
    }
    iter->stack = stack;
  }
  iter->i = 0;
  iter->n = frame->argc;
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
  g_return_val_if_fail (frame != NULL, NULL);

  context = swfdec_gc_object_get_context (frame->target);
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

/*** BLOCK HANDLING ***/

typedef struct {
  const guint8 *		start;	/* start of block */
  const guint8 *		end;	/* end of block (hitting this address will exit the block) */
  SwfdecAsFrameBlockFunc	func;	/* function to call when block is exited (or frame is destroyed) */
  gpointer			data;	/* data to pass to function */
} SwfdecAsFrameBlock;

/**
 * swfdec_as_frame_push_block:
 * @frame: a #SwfdecAsFrame
 * @start: start of block
 * @end: byte after end of block
 * @func: function to call when block gets exited
 * @data: data to pass to @func
 *
 * Registers a function that guards a block of memory. When the function 
 * exits this block of memory, it will call this function. This can happen
 * either when the program counter leaves the guarded region, when the 
 * function returns or when the context aborted due to an unrecoverable error.
 **/
void
swfdec_as_frame_push_block (SwfdecAsFrame *frame, const guint8 *start, 
    const guint8 *end, SwfdecAsFrameBlockFunc func, gpointer data)
{
  SwfdecAsFrameBlock block = { start, end, func, data };

  g_return_if_fail (frame != NULL);
  g_return_if_fail (start <= end);
  g_return_if_fail (start >= frame->block_start);
  g_return_if_fail (end <= frame->block_end);
  g_return_if_fail (func != NULL);

  frame->block_start = start;
  frame->block_end = end;
  g_array_append_val (frame->blocks, block);
}

void
swfdec_as_frame_pop_block (SwfdecAsFrame *frame, SwfdecAsContext *cx)
{
  SwfdecAsFrameBlockFunc func;
  gpointer data;
  SwfdecAsFrameBlock *block;

  g_return_if_fail (frame != NULL);
  g_return_if_fail (frame->blocks->len > 0);

  block = &g_array_index (frame->blocks, SwfdecAsFrameBlock, frame->blocks->len - 1);
  func = block->func;
  data = block->data;
  g_array_set_size (frame->blocks, frame->blocks->len - 1);
  if (frame->blocks->len) {
    block--;
    frame->block_start = block->start;
    frame->block_end = block->end;
  } else {
    frame->block_start = frame->script->buffer->data;
    frame->block_end = frame->script->buffer->data + frame->script->buffer->length;
  }
  /* only call function after we popped the block, so the block can push a new one */
  func (cx, frame, data);
}

/*** FRAME ***/

static void
swfdec_as_frame_free (SwfdecAsFrame *frame)
{
  /* pop blocks while state is intact */
  while (frame->blocks->len > 0)
    swfdec_as_frame_pop_block (frame, swfdec_gc_object_get_context (frame->target));

  /* clean up */
  g_slice_free1 (sizeof (SwfdecAsValue) * frame->n_registers, frame->registers);
  if (frame->constant_pool) {
    swfdec_constant_pool_unref (frame->constant_pool);
    frame->constant_pool = NULL;
  }
  g_array_free (frame->blocks, TRUE);
  g_slist_free (frame->scope_chain);
  if (frame->script) {
    swfdec_script_unref (frame->script);
    frame->script = NULL;
  }
}

void
swfdec_as_frame_init (SwfdecAsFrame *frame, SwfdecAsContext *context, SwfdecScript *script)
{
  g_return_if_fail (frame != NULL);
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (script != NULL);
  
  swfdec_as_frame_init_native (frame, context);
  frame->script = swfdec_script_ref (script);
  SWFDEC_DEBUG ("new frame for function %s", script->name);
  frame->pc = script->main;
  frame->n_registers = script->n_registers;
  frame->registers = g_slice_alloc0 (sizeof (SwfdecAsValue) * frame->n_registers);
  if (script->constant_pool) {
    frame->constant_pool = swfdec_constant_pool_new (context, 
	script->constant_pool, script->version);
    if (frame->constant_pool == NULL) {
      SWFDEC_ERROR ("couldn't create constant pool");
    }
  }
}

void
swfdec_as_frame_init_native (SwfdecAsFrame *frame, SwfdecAsContext *context)
{
  g_return_if_fail (frame != NULL);
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  
  frame->blocks = g_array_new (FALSE, FALSE, sizeof (SwfdecAsFrameBlock));
  frame->block_end = (gpointer) -1;
  frame->stack_begin = context->cur;
  context->base = frame->stack_begin;
  frame->next = context->frame;
  context->frame = frame;
  context->call_depth++;
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

  g_return_if_fail (frame != NULL);
  context = swfdec_gc_object_get_context (frame->target ? (gpointer) frame->target : (gpointer) frame->function);
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
  if (context->debugger) {
    SwfdecAsDebuggerClass *klass = SWFDEC_AS_DEBUGGER_GET_CLASS (context->debugger);

    if (klass->leave_frame)
      klass->leave_frame (context->debugger, context, frame, &retval);
  }
  /* set return value */
  if (frame->return_value) {
    *frame->return_value = retval;
  } else {
    swfdec_as_stack_ensure_free (context, 1);
    *swfdec_as_stack_push (context) = retval;
  }
  swfdec_as_frame_free (frame);
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
  g_return_if_fail (frame != NULL);
  g_return_if_fail (frame->thisp == NULL);
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (thisp));

  g_assert (!SWFDEC_IS_AS_SUPER (thisp));
  frame->thisp = thisp;
  if (frame->target == NULL) {
    frame->target = thisp;
    frame->original_target = thisp;
  }
}

/**
 * swfdec_as_frame_get_variable_and_flags:
 * @frame: a #SwfdecAsFrame
 * @variable: name of the variable
 * @value: pointer to take value of the variable or %NULL
 * @flags: pointer to take flags or %NULL
 * @pobject: pointer to take the actual object that held the variable or %NULL
 *
 * Walks the scope chain of @frame trying to resolve the given @variable and if
 * found, returns its value and flags. Note that there might be a difference 
 * between @pobject and the returned object, since the returned object will be
 * part of the scope chain while @pobject will contain the actual property. It
 * will be a prototype of the returned object though.
 *
 * Returns: Object in scope chain that contained the variable.
 **/
SwfdecAsObject *
swfdec_as_frame_get_variable_and_flags (SwfdecAsFrame *frame, const char *variable,
    SwfdecAsValue *value, guint *flags, SwfdecAsObject **pobject)
{
  GSList *walk;
  SwfdecAsContext *cx;

  g_return_val_if_fail (frame != NULL, NULL);
  g_return_val_if_fail (variable != NULL, NULL);

  cx = swfdec_gc_object_get_context (frame->target);

  for (walk = frame->scope_chain; walk; walk = walk->next) {
    if (swfdec_as_object_get_variable_and_flags (walk->data, variable, value, 
	  flags, pobject))
      return walk->data;
  }
  /* we've walked the scope chain down. Now look in the special objects. */
  /* 1) the target (if removed, use original target) */
  if (SWFDEC_IS_MOVIE (frame->target) &&
      SWFDEC_MOVIE(frame->target)->state < SWFDEC_MOVIE_STATE_DESTROYED) {
    if (swfdec_as_object_get_variable_and_flags (frame->target, variable,
	  value, flags, pobject))
      return frame->target;
  } else {
    if (swfdec_as_object_get_variable_and_flags (frame->original_target,
	  variable, value, flags, pobject))
      return frame->original_target;
  }
  /* 2) the global object */
  /* FIXME: ignored on version 4, but should it never be created instead? */
  if (cx->version > 4 && swfdec_as_object_get_variable_and_flags (cx->global,
	variable, value, flags, pobject))
    return cx->global;

  return NULL;
}

void
swfdec_as_frame_set_variable_and_flags (SwfdecAsFrame *frame, const char *variable,
    const SwfdecAsValue *value, guint default_flags, gboolean overwrite, gboolean local)
{
  SwfdecAsObject *pobject, *set;
  GSList *walk;

  g_return_if_fail (frame != NULL);
  g_return_if_fail (variable != NULL);

  set = NULL;
  for (walk = frame->scope_chain; walk; walk = walk->next) {
    if (swfdec_as_object_get_variable_and_flags (walk->data, variable, NULL, NULL, &pobject) &&
	pobject == walk->data) {
      if (!overwrite)
	return;
      set = walk->data;
      break;
    }
  }
  if (set == NULL) {
    if (local && frame->activation) {
      set = frame->activation;
    } else {
      set = frame->target;
    }
  }

  if (!overwrite) {
    if (swfdec_as_object_get_variable (set, variable, NULL))
      return;
  }

  swfdec_as_object_set_variable_and_flags (set, variable, value, default_flags);
}

SwfdecAsDeleteReturn
swfdec_as_frame_delete_variable (SwfdecAsFrame *frame, const char *variable)
{
  GSList *walk;
  SwfdecAsDeleteReturn ret;

  g_return_val_if_fail (frame != NULL, FALSE);
  g_return_val_if_fail (variable != NULL, FALSE);

  for (walk = frame->scope_chain; walk; walk = walk->next) {
    ret = swfdec_as_object_delete_variable (walk->data, variable);
    if (ret)
      return ret;
  }
  /* we've walked the scope chain down. Now look in the special objects. */
  /* 1) the target set via SetTarget */
  ret = swfdec_as_object_delete_variable (frame->target, variable);
  if (ret)
    return ret;
  /* 2) the global object */
  return swfdec_as_object_delete_variable (swfdec_gc_object_get_context (frame->target)->global, variable);
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
  g_return_if_fail (frame != NULL);
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
  SwfdecAsObject *object, *args;
  guint i, current_reg = 1;
  SwfdecScript *script;
  SwfdecAsValue val;
  const SwfdecAsValue *cur;
  SwfdecAsContext *context;
  SwfdecAsStackIterator iter;

  g_return_if_fail (frame != NULL);

  /* setup */
  context = swfdec_gc_object_get_context (frame->target ? (gpointer) frame->target : (gpointer) frame->function);
  script = frame->script;
  if (frame->script == NULL)
    goto out;
  frame->activation = swfdec_as_object_new_empty (context);
  object = frame->activation;
  frame->scope_chain = g_slist_prepend (frame->scope_chain, object);

  /* create arguments and super object if necessary */
  if ((script->flags & (SWFDEC_SCRIPT_PRELOAD_ARGS | SWFDEC_SCRIPT_SUPPRESS_ARGS)) != SWFDEC_SCRIPT_SUPPRESS_ARGS) {
    SwfdecAsFrame *next;
    args = swfdec_as_array_new (context);
    for (cur = swfdec_as_stack_iterator_init_arguments (&iter, frame); cur != NULL;
	cur = swfdec_as_stack_iterator_next (&iter)) {
      swfdec_as_array_push (args, cur);
    }

    next = frame->next;
    while (next != NULL && (next->function == NULL ||
	SWFDEC_IS_AS_NATIVE_FUNCTION (next->function))) {
      next = next->next;
    }
    if (next != NULL) {
      SWFDEC_AS_VALUE_SET_OBJECT (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (next->function)));
    } else {
      SWFDEC_AS_VALUE_SET_NULL (&val);
    }
    swfdec_as_object_set_variable_and_flags (args, SWFDEC_AS_STR_caller, &val,
	SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);

    if (frame->function != NULL) {
      SWFDEC_AS_VALUE_SET_OBJECT (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (frame->function)));
    } else {
      SWFDEC_AS_VALUE_SET_NULL (&val);
    }
    swfdec_as_object_set_variable_and_flags (args, SWFDEC_AS_STR_callee, &val,
	SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  } else {
    /* silence gcc */
    args = NULL;
  }

  /* set the default variables (unless suppressed */
  if (!(script->flags & SWFDEC_SCRIPT_SUPPRESS_THIS)) {
    if (frame->thisp) {
      SWFDEC_AS_VALUE_SET_OBJECT (&val, frame->thisp);
    } else {
      SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
    }
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_this, &val);
  }
  if (!(script->flags & SWFDEC_SCRIPT_SUPPRESS_ARGS)) {
    SWFDEC_AS_VALUE_SET_OBJECT (&val, args);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_arguments, &val);
  }
  if (!(script->flags & SWFDEC_SCRIPT_SUPPRESS_SUPER)) {
    if (frame->super) {
      SWFDEC_AS_VALUE_SET_OBJECT (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (frame->super)));
    } else {
      SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
    }
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_super, &val);
  }

  /* set and preload argument variables */
  SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  cur = swfdec_as_stack_iterator_init_arguments (&iter, frame);
  for (i = 0; i < script->n_arguments; i++) {
    if (cur == NULL)
      cur = &val;
    /* set this value at the right place */
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
    /* get the next argument */
    cur = swfdec_as_stack_iterator_next (&iter);
  }

  /* preload from flags */
  if ((script->flags & (SWFDEC_SCRIPT_PRELOAD_THIS | SWFDEC_SCRIPT_SUPPRESS_THIS)) == SWFDEC_SCRIPT_PRELOAD_THIS
      && current_reg < script->n_registers) {
    if (frame->thisp) {
      SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], frame->thisp);
    } else {
      SWFDEC_AS_VALUE_SET_UNDEFINED (&frame->registers[current_reg++]);
    }
  }
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_ARGS && current_reg < script->n_registers) {
    SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], args);
  }
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_SUPER && current_reg < script->n_registers) {
    if (frame->super) {
      SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], 
	  swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (frame->super)));
    } else {
      SWFDEC_AS_VALUE_SET_UNDEFINED (&frame->registers[current_reg++]);
    }
  }
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_ROOT && current_reg < script->n_registers) {
    if (!swfdec_as_frame_get_variable (frame, SWFDEC_AS_STR__root, &frame->registers[current_reg])) {
      SWFDEC_WARNING ("no root to preload");
    }
    current_reg++;
  }
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_PARENT && current_reg < script->n_registers) {
    if (!swfdec_as_frame_get_variable (frame, SWFDEC_AS_STR__parent, &frame->registers[current_reg])) {
      SWFDEC_WARNING ("no root to preload");
    }
    current_reg++;
  }
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_GLOBAL && current_reg < script->n_registers) {
    SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], context->global);
  }
  /* set block boundaries */
  frame->block_start = frame->script->buffer->data;
  frame->block_end = frame->script->buffer->data + frame->script->buffer->length;

out:
  if (context->state == SWFDEC_AS_CONTEXT_ABORTED) {
    swfdec_as_frame_return (frame, NULL);
    return;
  }
  if (context->debugger) {
    SwfdecAsDebuggerClass *klass = SWFDEC_AS_DEBUGGER_GET_CLASS (context->debugger);

    if (klass->enter_frame)
      klass->enter_frame (context->debugger, context, frame);
  }
}

void
swfdec_as_frame_handle_exception (SwfdecAsFrame *frame)
{
  SwfdecAsContext *cx;

  g_return_if_fail (frame != NULL);
  cx = swfdec_gc_object_get_context (frame->target);
  g_return_if_fail (cx->exception);

  /* pop blocks in the hope that we are inside a Try block */
  while (cx->exception && frame->blocks->len) {
    swfdec_as_frame_pop_block (frame, cx);
  }
  /* no Try blocks caught it, exit frame */
  if (cx->exception) {
    swfdec_as_frame_return (frame, NULL);
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
  g_return_val_if_fail (frame != NULL, NULL);

  return frame->next;
}

/**
 * swfdec_as_frame_get_script:
 * @frame: a #SwfdecAsFrame
 *
 * Gets the script associated with the given @frame. If the frame references
 * a native function, there will be no script and this function returns %NULL.
 *
 * Returns: The script executed by this frame or %NULL
 **/
SwfdecScript *
swfdec_as_frame_get_script (SwfdecAsFrame *frame)
{
  g_return_val_if_fail (frame != NULL, NULL);

  return frame->script;
}

/**
 * swfdec_as_frame_get_this:
 * @frame: a #SwfdecAsFrame
 *
 * Gets the this object of the given @frame. If the frame has no this object,
 * %NULL is returned.
 *
 * Returns: The this object of the frame or %NULL if none.
 **/
SwfdecAsObject *
swfdec_as_frame_get_this (SwfdecAsFrame *frame)
{
  g_return_val_if_fail (frame != NULL, NULL);

  return frame->thisp;
}

