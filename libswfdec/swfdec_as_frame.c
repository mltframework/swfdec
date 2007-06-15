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

#include "swfdec_as_frame.h"
#include "swfdec_as_array.h"
#include "swfdec_as_context.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_super.h"
#include "swfdec_debug.h"

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
  if (frame->stack) {
    swfdec_as_stack_free (frame->stack);
    frame->stack = NULL;
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
  if (frame->script) {
    swfdec_as_object_mark (frame->var_object);
  }
  if (frame->thisp)
    swfdec_as_object_mark (frame->thisp);
  if (frame->target)
    swfdec_as_object_mark (frame->target);
  if (frame->function)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (frame->function));
  for (i = 0; i < frame->n_registers; i++) {
    swfdec_as_value_mark (&frame->registers[i]);
  }
  /* don't mark argv, it's const, others have to take care of it */
  swfdec_as_stack_mark (frame->stack);
  SWFDEC_AS_OBJECT_CLASS (swfdec_as_frame_parent_class)->mark (object);
}

static void
swfdec_as_frame_class_init (SwfdecAsFrameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_frame_dispose;

  asobject_class->mark = swfdec_as_frame_mark;
}

static void
swfdec_as_frame_init (SwfdecAsFrame *frame)
{
  frame->function_name = "unnamed";
}

SwfdecAsFrame *
swfdec_as_frame_new (SwfdecAsContext *context, SwfdecScript *script)
{
  SwfdecAsFrame *frame;
  SwfdecAsStack *stack;
  gsize size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (script != NULL, NULL);
  
  stack = swfdec_as_stack_new (context, 100); /* FIXME: invent better numbers here */
  if (!stack)
    return NULL;
  size = sizeof (SwfdecAsFrame) + sizeof (SwfdecAsValue) * script->n_registers;
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  frame = g_object_new (SWFDEC_TYPE_AS_FRAME, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (frame), context, size);
  frame->script = swfdec_script_ref (script);
  frame->function_name = script->name;
  SWFDEC_DEBUG ("new frame for function %s", frame->function_name);
  frame->pc = script->buffer->data;
  frame->stack = stack;
  frame->scope = SWFDEC_AS_SCOPE (frame);
  if (frame->next)
    frame->var_object = frame->next->var_object;
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
  return frame;
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
  if (frame->var_object == NULL)
    frame->var_object = thisp;
}

/**
 * swfdec_as_frame_find_variable:
 * @frame: a #SwfdecAsFrame
 * @variable: name of the variable to find
 *
 * Finds the given variable in the current scope chain. Returns the first 
 * object in the scope chain that contains this variable in its prototype 
 * chain. If you want to know the explicit object that contains the variable,
 * you have to call swfdec_as_object_find_variable() on the result.
 * If no such variable exist in the scope chain, %NULL is returned.
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
  /* 1) the target set via SetTarget */
  if (frame->target) {
    if (swfdec_as_object_get_variable (frame->target, variable, &val))
      return frame->target;
  } else {
    /* The default target is the original object that called into us */
    if (swfdec_as_object_get_variable (SWFDEC_AS_FRAME (cur)->thisp, variable, &val))
      return SWFDEC_AS_FRAME (cur)->thisp;
  }
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
  if (frame->target) {
    if (swfdec_as_object_delete_variable (frame->target, variable))
      return TRUE;
  } else {
    /* The default target is the original object that called into us */
    if (swfdec_as_object_delete_variable (SWFDEC_AS_FRAME (cur)->thisp, variable))
      return TRUE;
  }
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

  frame->target = target;
}

void
swfdec_as_frame_preload (SwfdecAsFrame *frame)
{
  SwfdecAsObject *object;
  guint i, current_reg = 1;
  SwfdecScript *script;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_FRAME (frame));

  if (frame->script == NULL)
    return;

  object = SWFDEC_AS_OBJECT (frame);
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
    SwfdecAsObject *args = swfdec_as_array_new (object->context);

    if (!args)
      return;
    if (frame->argc > 0)
      swfdec_as_array_append (SWFDEC_AS_ARRAY (args), frame->argc, frame->argv);
    /* FIXME: implement callee/caller */
    if (script->flags & SWFDEC_SCRIPT_PRELOAD_ARGS) {
      SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], args);
    } else {
      SWFDEC_AS_VALUE_SET_OBJECT (&val, args);
      swfdec_as_object_set_variable (object, SWFDEC_AS_STR_arguments, &val);
    }
  }
  if (!(script->flags & SWFDEC_SCRIPT_SUPPRESS_SUPER)) {
    SwfdecAsObject *super = swfdec_as_super_new (frame);
    if (super) {
      if (script->flags & SWFDEC_SCRIPT_PRELOAD_SUPER) {
	SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], super);
      } else {
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
      SWFDEC_WARNING ("no parentto preload");
    }
    current_reg++;
  }
  if (script->flags & SWFDEC_SCRIPT_PRELOAD_GLOBAL) {
    SWFDEC_AS_VALUE_SET_OBJECT (&frame->registers[current_reg++], object->context->global);
  }
  for (i = 0; i < script->n_arguments; i++) {
    if (script->arguments[i].preload) {
      /* the script is responsible for ensuring this */
      g_assert (script->arguments[i].preload < frame->n_registers);
      if (i < frame->argc) {
	frame->registers[script->arguments[i].preload] = frame->argv[i];
      }
    } else {
      const char *tmp = swfdec_as_context_get_string (object->context, script->arguments[i].name);
      if (i < frame->argc) {
	swfdec_as_object_set_variable (object, tmp, &frame->argv[i]);
      } else {
	SwfdecAsValue val = { 0, };
	swfdec_as_object_set_variable (object, tmp, &val);
      }
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

