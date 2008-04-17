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
#include "vivi_code_undefined.h"
#include "vivi_decompiler_unknown.h"

struct _ViviDecompilerState {
  SwfdecScript *	script;
  const guint8 *	pc;
  ViviCodeValue **	registers;
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
      g_object_unref (state->registers[i]);
  }
  g_slice_free1 (sizeof (ViviCodeValue *) * state->n_registers, state->registers);
  g_slist_foreach (state->stack, (GFunc) g_object_unref, NULL);
  g_slist_free (state->stack);
  if (state->pool)
    swfdec_constant_pool_unref (state->pool);
  swfdec_script_unref (state->script);
  g_slice_free (ViviDecompilerState, state);
}

ViviDecompilerState *
vivi_decompiler_state_new (SwfdecScript *script, const guint8 *pc, guint n_registers)
{
  ViviDecompilerState *state = g_slice_new0 (ViviDecompilerState);

  state->script = swfdec_script_ref (script);
  state->pc = pc;
  state->registers = g_slice_alloc0 (sizeof (ViviCodeValue *) * n_registers);
  state->n_registers = n_registers;

  return state;
}

void
vivi_decompiler_state_push (ViviDecompilerState *state, ViviCodeValue *val)
{
  state->stack = g_slist_prepend (state->stack, val);
}

ViviCodeValue *
vivi_decompiler_state_pop (ViviDecompilerState *state)
{
  if (state->stack == NULL) {
    return vivi_code_undefined_new ();
  } else {
    ViviCodeValue *pop;
    pop = state->stack->data;
    state->stack = g_slist_remove (state->stack, pop);
    return pop;
  }
}

guint
vivi_decompiler_state_get_stack_depth (const ViviDecompilerState *state)
{
  g_return_val_if_fail (state != NULL, 0);

  return g_slist_length (state->stack);
}

ViviCodeValue *
vivi_decompiler_state_peek_nth (const ViviDecompilerState *state, guint i)
{
  g_return_val_if_fail (state != NULL, NULL);

  return g_slist_nth_data (state->stack, i);
}

ViviDecompilerState *
vivi_decompiler_state_copy (const ViviDecompilerState *src)
{
  ViviDecompilerState *dest;
  guint i;

  dest = vivi_decompiler_state_new (src->script, src->pc, src->n_registers);
  for (i = 0; i < src->n_registers; i++) {
    if (src->registers[i])
      dest->registers[i] = g_object_ref (src->registers[i]);
  }
  dest->stack = g_slist_copy (src->stack);
  g_slist_foreach (dest->stack, (GFunc) g_object_ref, NULL);
  if (src->pool)
    dest->pool = swfdec_constant_pool_ref (src->pool);

  return dest;
}

ViviCodeValue *
vivi_decompiler_state_get_register (const ViviDecompilerState *state, guint reg)
{
  if (reg >= state->n_registers || state->registers[reg] == NULL)
    return vivi_code_undefined_new ();
  else
    return g_object_ref (state->registers[reg]);
}

void
vivi_decompiler_state_set_register (ViviDecompilerState *state, guint reg,
    ViviCodeValue *value)
{
  g_return_if_fail (state != NULL);
  g_return_if_fail (VIVI_IS_CODE_VALUE (value));

  if (reg >= state->n_registers) {
    g_printerr ("register %u out of range [0,%u]\n", reg, state->n_registers);
    return;
  }

  if (state->registers[reg] != NULL)
    g_object_unref (state->registers[reg]);
  state->registers[reg] = g_object_ref (value);
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

SwfdecScript *
vivi_decompiler_state_get_script (const ViviDecompilerState *state)
{
  return state->script;
}

SwfdecConstantPool *
vivi_decompiler_state_get_constant_pool (const ViviDecompilerState *state)
{
  return state->pool;
}

void
vivi_decompiler_state_set_constant_pool (ViviDecompilerState *state, 
    SwfdecConstantPool *pool)
{
  if (state->pool)
    swfdec_constant_pool_unref (state->pool);
  state->pool = pool;
}

guint
vivi_decompiler_state_get_version (const ViviDecompilerState *state)
{
  return swfdec_script_get_version (state->script);
}

gboolean
vivi_decompiler_state_is_compatible (const ViviDecompilerState *a, const ViviDecompilerState *b)
{
  g_return_val_if_fail (a != NULL, FALSE);
  g_return_val_if_fail (b != NULL, FALSE);

  if (a->n_registers != b->n_registers)
    return FALSE;
  /* FIXME: check registers */
  if (a->pool || b->pool) {
    guint i, len;

    if (a->pool == NULL || b->pool == NULL)
      return FALSE;
    len = swfdec_constant_pool_size (a->pool);
    if (len != swfdec_constant_pool_size (b->pool))
      return FALSE;
    for (i = 0; i < len; i++) {
      if (!g_str_equal (swfdec_constant_pool_get (a->pool, i),
	    swfdec_constant_pool_get (b->pool, i)))
	return FALSE;
    }
  }
  /* FIXME: is this necessary? */
  if (g_slist_length (a->stack) != g_slist_length (b->stack))
    return FALSE;

  return TRUE;
}

