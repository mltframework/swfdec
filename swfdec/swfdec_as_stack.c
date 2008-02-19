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

#include <string.h>
#include "swfdec_as_stack.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_debug.h"

/* FIXME: make this configurable? */
#define SWFDEC_AS_STACK_SIZE 8192 /* random big number */

/* minimum number of elements that need to stay free that makes us remove a stack segment */
#define SWFDEC_AS_STACK_LEFTOVER 16

static SwfdecAsStack *
swfdec_as_stack_new (SwfdecAsContext *context, guint n_elements)
{
  gsize size;
  SwfdecAsStack *stack;

  size = sizeof (SwfdecAsStack) + n_elements * sizeof (SwfdecAsValue);
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;

  stack = g_slice_alloc (size);
  stack->n_elements = n_elements;
  stack->used_elements = 0;
  stack->next = NULL;
  return stack;
}

static void
swfdec_as_stack_set (SwfdecAsContext *context, SwfdecAsStack *stack)
{
  context->stack = stack;
  context->base = &stack->elements[0];
  context->cur = context->base + stack->used_elements;
  context->end = context->base + SWFDEC_AS_STACK_SIZE;
}

gboolean
swfdec_as_stack_push_segment (SwfdecAsContext *context)
{
  SwfdecAsStack *stack;

  /* finish current stack */
  if (context->stack) {
    context->stack->used_elements = context->cur - context->base;
    g_assert (context->stack->used_elements <= context->stack->n_elements);
  }

  stack = swfdec_as_stack_new (context, SWFDEC_AS_STACK_SIZE);
  if (stack == NULL)
    return FALSE;
  SWFDEC_DEBUG ("pushing new stack segment %p", stack);
  stack->next = context->stack;
  swfdec_as_stack_set (context, stack);
  return TRUE;
}

static void
swfdec_as_stack_free (SwfdecAsContext *context, SwfdecAsStack *stack)
{
  gsize size;

  size = sizeof (SwfdecAsStack) + stack->n_elements * sizeof (SwfdecAsValue);
  swfdec_as_context_unuse_mem (context, size);
  g_slice_free1 (size, stack);
}

void
swfdec_as_stack_pop_segment (SwfdecAsContext *context)
{
  SwfdecAsStack *stack = context->stack;
  if (stack->next) {
    swfdec_as_stack_set (context, stack->next);
  } else {
    context->base = context->cur = context->end = NULL;
    context->stack = NULL;
  }
  SWFDEC_DEBUG ("popping stack segment %p, next is %p", stack, context->stack);
  swfdec_as_stack_free (context, stack);
}

void
swfdec_as_stack_mark (SwfdecAsStack *stack)
{
  guint i;

  for (i = 0; i < stack->used_elements; i++) {
    swfdec_as_value_mark (&stack->elements[i]);
  }
  if (stack->next)
    swfdec_as_stack_mark (stack->next);
}

void
swfdec_as_stack_ensure_size (SwfdecAsContext *context, guint n_elements)
{
  guint current;
  SwfdecAsStack *next;
  
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (n_elements < SWFDEC_AS_STACK_SIZE / 2);

  current = (guint) (context->cur - context->base);
  if (G_LIKELY (current >= n_elements))
    return;
  next = context->stack->next;
  /* check if we can move this to the last stack */
  if (next && context->base != context->frame->stack_begin &&
      (next->n_elements - next->used_elements > SWFDEC_AS_STACK_LEFTOVER + current)) {
    memmove (&next->elements[next->used_elements], context->base, current * sizeof (SwfdecAsValue));
    next->used_elements += current;
    swfdec_as_stack_pop_segment (context);
    return;
  }
  if (current) {
    n_elements -= current;
    memmove (context->base + n_elements, context->base, current * sizeof (SwfdecAsValue));
  }
  context->cur += n_elements;
  if (n_elements) {
    if (next && context->base != context->frame->stack_begin) {
      /* this should be true by a huge amount */
      g_assert (next->used_elements >= n_elements);
      if (context->frame->stack_begin <= &next->elements[next->used_elements] &&
	  context->frame->stack_begin >= &next->elements[0]) {
	current = &next->elements[next->used_elements] - context->frame->stack_begin;
	current = MIN (n_elements, current);
      } else {
	current = n_elements;
      }
      next->used_elements -= current;
      n_elements -= current;
      memmove (context->base + n_elements, &next->elements[next->used_elements], current * sizeof (SwfdecAsValue));
    }
    if (n_elements) {
      memset (context->base, 0, n_elements * sizeof (SwfdecAsValue));
    }
  }
}

void
swfdec_as_stack_ensure_free (SwfdecAsContext *context, guint n_elements)
{
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (n_elements < SWFDEC_AS_STACK_SIZE / 2);

  if (G_LIKELY ((guint) (context->end - context->cur) >= n_elements))
    return;

  if (!swfdec_as_stack_push_segment (context))
    context->cur = context->end - n_elements;
}

guint
swfdec_as_stack_get_size (SwfdecAsContext *context)
{
  SwfdecAsStack *stack;
  SwfdecAsValue *target;
  guint ret;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), 0);

  if (context->frame == NULL)
    return 0;
  target = context->frame->stack_begin;
  if (target == context->base)
    return context->cur - context->base;
  stack = context->stack->next;
  ret = context->cur - context->base;
  while (target < &stack->elements[0] && target > &stack->elements[stack->used_elements]) {
    ret += stack->n_elements;
    stack = stack->next;
  }
  ret += &stack->elements[stack->used_elements] - target;
  return ret;
}
