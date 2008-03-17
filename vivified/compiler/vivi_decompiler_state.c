/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include <swfdec/swfdec_script_internal.h>

#include "vivi_decompiler_state.h"

struct _ViviDecompilerState {
  SwfdecScript *	script;
  const guint8 *	pc;
  ViviDecompilerValue **registers;
  guint			n_registers;
  GSList *		stack;
  SwfdecConstantPool *	pool;
};

void
vivi_decompiler_state_free (ViviDecompilerState *state)
{
  guint i;

  for (i = 0; i < state->n_registers; i++) {
    if (state->registers[i])
      vivi_decompiler_value_free (state->registers[i]);
  }
  g_slice_free1 (sizeof (ViviDecompilerValue *) * state->n_registers, state->registers);
  g_slist_foreach (state->stack, (GFunc) vivi_decompiler_value_free, NULL);
  g_slist_free (state->stack);
  if (state->pool)
    swfdec_constant_pool_free (state->pool);
  swfdec_script_unref (state->script);
  g_slice_free (ViviDecompilerState, state);
}

ViviDecompilerState *
vivi_decompiler_state_new (SwfdecScript *script, const guint8 *pc, guint n_registers)
{
  ViviDecompilerState *state = g_slice_new0 (ViviDecompilerState);

  state->script = swfdec_script_ref (script);
  state->pc = pc;
  state->registers = g_slice_alloc0 (sizeof (ViviDecompilerValue *) * n_registers);
  state->n_registers = n_registers;

  return state;
}

void
vivi_decompiler_state_push (ViviDecompilerState *state, ViviDecompilerValue *val)
{
  state->stack = g_slist_prepend (state->stack, val);
}

ViviDecompilerValue *
vivi_decompiler_state_pop (ViviDecompilerState *state)
{
  if (state->stack == NULL) {
    return vivi_decompiler_value_copy (vivi_decompiler_value_get_undefined ());
  } else {
    ViviDecompilerValue *pop;
    pop = state->stack->data;
    state->stack = g_slist_remove (state->stack, pop);
    return pop;
  }
}

ViviDecompilerState *
vivi_decompiler_state_copy (const ViviDecompilerState *src)
{
  ViviDecompilerState *dest;
  guint i;
  GSList *walk;

  dest = vivi_decompiler_state_new (src->script, src->pc, src->n_registers);
  for (i = 0; i < src->n_registers; i++) {
    if (src->registers[i])
      dest->registers[i] = vivi_decompiler_value_copy (src->registers[i]);
  }
  for (walk = src->stack; walk; walk = walk->next) {
    dest->stack = g_slist_prepend (dest->stack, vivi_decompiler_value_copy (walk->data));
  }
  dest->stack = g_slist_reverse (dest->stack);
  if (src->pool)
    dest->pool = swfdec_constant_pool_copy (src->pool);

  return dest;
}

const ViviDecompilerValue *
vivi_decompiler_state_get_register (const ViviDecompilerState *state, guint reg)
{
  if (reg >= state->n_registers || state->registers[state->n_registers] == NULL)
    return vivi_decompiler_value_get_undefined ();
  else
    return state->registers[state->n_registers];
}

const guint8 *
vivi_decompiler_state_get_pc (const ViviDecompilerState *state)
{
  return state->pc;
}

void
vivi_decompiler_state_add_pc (ViviDecompilerState *state, int diff)
{
  state->pc += diff;
}

const SwfdecConstantPool *
vivi_decompiler_state_get_constant_pool (const ViviDecompilerState *state)
{
  return state->pool;
}

void
vivi_decompiler_state_set_constant_pool (ViviDecompilerState *state, 
    SwfdecConstantPool *pool)
{
  if (state->pool)
    swfdec_constant_pool_free (state->pool);
  state->pool = pool;
}

guint
vivi_decompiler_state_get_version (const ViviDecompilerState *state)
{
  return swfdec_script_get_version (state->script);
}

