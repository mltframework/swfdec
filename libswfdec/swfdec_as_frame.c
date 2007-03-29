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
  for (i = 0; i < frame->n_registers; i++) {
    swfdec_as_value_mark (&frame->registers[i]);
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
}

SwfdecAsFrame *
swfdec_as_frame_new (SwfdecAsContext *context, SwfdecAsObject *thisp, SwfdecScript *script)
{
  SwfdecAsValue val;
  SwfdecAsFrame *frame;
  SwfdecAsStack *stack;
  gsize size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (thisp), NULL);
  g_return_val_if_fail (script != NULL, NULL);
  
  stack = swfdec_as_stack_new (context, 100); /* FIXME: invent better numbers here */
  if (!stack)
    return NULL;
  size = sizeof (SwfdecAsObject) + sizeof (SwfdecAsValue) * script->n_registers;
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  frame = g_object_new (SWFDEC_TYPE_AS_FRAME, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (frame), context, size);
  frame->next = context->frame;
  context->frame = frame;
  frame->script = swfdec_script_ref (script);
  frame->pc = script->buffer->data;
  frame->stack = stack;
  frame->scope = thisp;
  frame->var_object = thisp;
  frame->registers = g_slice_alloc0 (sizeof (SwfdecAsValue) * script->n_registers);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, thisp);
  swfdec_as_object_set (SWFDEC_AS_OBJECT (frame), SWFDEC_AS_STR_THIS, &val);
  return frame;
}

