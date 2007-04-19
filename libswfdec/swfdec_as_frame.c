/* SwfdecAs
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
#include "swfdec_as_context.h"
#include "swfdec_as_stack.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsFrame, swfdec_as_frame, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_frame_dispose (GObject *object)
{
  SwfdecAsFrame *frame = SWFDEC_AS_FRAME (object);

  g_slice_free1 (sizeof (SwfdecAsValue) * frame->n_registers, frame->registers);
  swfdec_script_unref (frame->script);
  swfdec_as_stack_free (frame->stack);
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

  swfdec_as_object_mark (SWFDEC_AS_OBJECT (frame->next));
  swfdec_as_object_mark (frame->scope);
  swfdec_as_object_mark (frame->var_object);
  if (frame->target)
    swfdec_as_object_mark (frame->target);
  if (frame->function)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (frame->function));
  for (i = 0; i < frame->n_registers; i++) {
    swfdec_as_value_mark (&frame->registers[i]);
  }
  /* FIXME: do we want this? */
  for (i = 0; i < frame->argc; i++) {
    swfdec_as_value_mark (&frame->argv[i]);
  }
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
swfdec_as_frame_new (SwfdecAsObject *thisp, SwfdecScript *script)
{
  SwfdecAsValue val;
  SwfdecAsContext *context;
  SwfdecAsFrame *frame;
  SwfdecAsStack *stack;
  gsize size;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (thisp), NULL);
  g_return_val_if_fail (thisp->properties, NULL);
  g_return_val_if_fail (script != NULL, NULL);
  
  context = thisp->context;
  stack = swfdec_as_stack_new (context, 100); /* FIXME: invent better numbers here */
  if (!stack)
    return NULL;
  size = sizeof (SwfdecAsFrame) + sizeof (SwfdecAsValue) * script->n_registers;
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  frame = g_object_new (SWFDEC_TYPE_AS_FRAME, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (frame), context, size);
  frame->next = context->frame;
  context->frame = frame;
  frame->script = swfdec_script_ref (script);
  frame->function_name = script->name;
  SWFDEC_DEBUG ("new frame for function %s", frame->function_name);
  frame->pc = script->buffer->data;
  frame->stack = stack;
  frame->scope = thisp;
  frame->var_object = thisp;
  frame->n_registers = script->n_registers;
  frame->registers = g_slice_alloc0 (sizeof (SwfdecAsValue) * frame->n_registers);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, thisp);
  swfdec_as_object_set (SWFDEC_AS_OBJECT (frame), SWFDEC_AS_STR_THIS, &val);
  if (script->constant_pool) {
    frame->constant_pool_buffer = swfdec_buffer_ref (script->constant_pool);
    frame->constant_pool = swfdec_constant_pool_new_from_action (
	script->constant_pool->data, script->constant_pool->length);
    if (frame->constant_pool) {
      swfdec_constant_pool_attach_to_context (frame->constant_pool, context);
    } else {
      SWFDEC_ERROR ("couldn't create constant pool");
    }
  }
  return frame;
}

SwfdecAsFrame *
swfdec_as_frame_new_native (SwfdecAsObject *thisp)
{
  SwfdecAsContext *context;
  SwfdecAsFrame *frame;
  gsize size;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (thisp), NULL);
  g_return_val_if_fail (thisp->properties, NULL);
  
  context = thisp->context;
  size = sizeof (SwfdecAsFrame);
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  frame = g_object_new (SWFDEC_TYPE_AS_FRAME, NULL);
  SWFDEC_DEBUG ("new native frame");
  swfdec_as_object_add (SWFDEC_AS_OBJECT (frame), context, size);
  frame->next = context->frame;
  context->frame = frame;
  return frame;
}

SwfdecAsObject *
swfdec_as_frame_find_variable (SwfdecAsFrame *frame, const SwfdecAsValue *variable)
{
  SwfdecAsObject *cur, *ret = NULL;
  guint i;

  g_return_val_if_fail (SWFDEC_IS_AS_FRAME (frame), NULL);
  g_return_val_if_fail (SWFDEC_IS_AS_VALUE (variable), NULL);

  cur = SWFDEC_AS_OBJECT (frame);
  for (i = 0; i < 256 && frame != NULL; i++) {
    if (!SWFDEC_IS_AS_FRAME (cur))
      break;
    ret = swfdec_as_object_find_variable (cur, variable);
    if (ret)
      return ret;
    cur = SWFDEC_AS_FRAME (cur)->scope;
  }
  g_assert (cur);
  if (i == 256) {
    swfdec_as_context_abort (SWFDEC_AS_OBJECT (frame)->context, "Scope recursion limit exceeded");
    return NULL;
  }
  /* we've walked the scope chain down until the last object now.
   * The last 2 objects in the scope chain are the current target and the global object */
  if (frame->target) {
    ret = swfdec_as_object_find_variable (frame->target, variable);
  } else {
    ret = swfdec_as_object_find_variable (cur, variable);
  }
  if (ret)
    return ret;
  ret = swfdec_as_object_find_variable (cur->context->global, variable);
  return ret;
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

