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

#include <string.h>
#include "swfdec_as_stack.h"
#include "swfdec_as_context.h"

SwfdecAsStack *
swfdec_as_stack_new (SwfdecAsContext *context, guint n_elements)
{
  SwfdecAsStack *stack;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (n_elements > 0, NULL);

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsStack) +
	n_elements * sizeof (SwfdecAsValue)))
    return NULL;

  stack = g_slice_new (SwfdecAsStack);
  stack->context = context;
  stack->base = g_slice_alloc (sizeof (SwfdecAsValue) * n_elements);
  stack->end = stack->base + n_elements;
  stack->cur = stack->base;
  stack->next = NULL;
  return stack;
}

void
swfdec_as_stack_free (SwfdecAsStack *stack)
{
  g_return_if_fail (stack != NULL);

  swfdec_as_context_unuse_mem (stack->context, sizeof (SwfdecAsStack) + 
      (stack->end - stack->base) * sizeof (SwfdecAsValue));
  g_slice_free1 ((stack->end - stack->base) * sizeof (SwfdecAsValue), stack->base);
  g_slice_free (SwfdecAsStack, stack);
}

void
swfdec_as_stack_mark (SwfdecAsStack *stack)
{
  SwfdecAsValue *value;

  for (value = stack->base; value < stack->cur; value++) {
    swfdec_as_value_mark (value);
  }
}

void
swfdec_as_stack_ensure_size (SwfdecAsStack *stack, guint n_elements)
{
  guint current;

  g_return_if_fail (stack != NULL);
  g_return_if_fail (n_elements > (guint) (stack->end - stack->base));

  current = (guint) (stack->cur - stack->base);
  if (current) {
    n_elements -= current;
    memmove (stack->base + n_elements, stack->base, current * sizeof (SwfdecAsValue));
  }
  stack->cur += n_elements;
  if (n_elements)
    memset (stack->cur - n_elements, 0, n_elements * sizeof (SwfdecAsValue));
}

void
swfdec_as_stack_ensure_left (SwfdecAsStack *stack, guint n_elements)
{
  g_return_if_fail (stack != NULL);

  if ((guint) (stack->end - stack->cur) < n_elements) {
    /* FIXME FIXME FIXME */
    swfdec_as_context_abort (stack->context, "Out of stack space");
    stack->cur = stack->end - n_elements;
  }
}

