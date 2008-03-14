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

#include "vivi_decompiler.h"

/*** DECOMPILER ENGINE ***/

typedef struct _ViviDecompilerValue ViviDecompilerValue;
struct _ViviDecompilerValue {
  char *	value;
  gboolean	unconstant;
};

static void
vivi_decompiler_value_reset (ViviDecompilerValue *value)
{
  g_free (value->value);
  value->value = NULL;
  value->unconstant = TRUE;
}

typedef struct _ViviDecompilerState ViviDecompilerState;
struct _ViviDecompilerState {
  ViviDecompilerValue *	registers;
  guint			n_registers;
  GArray *		stack;
};

static void
vivi_decompiler_state_free (ViviDecompilerState *state)
{
  guint i;

  for (i = 0; i < state->n_registers; i++) {
    vivi_decompiler_value_reset (&state->registers[i]);
  }
  for (i = 0; i < state->stack->len; i++) {
    vivi_decompiler_value_reset (&g_array_index (state->stack, ViviDecompilerValue, i));
  }

  g_array_free (state->stack, TRUE);
  g_slice_free1 (sizeof (ViviDecompilerValue) * state->n_registers, state->registers);
  g_slice_free (ViviDecompilerState, state);
}

static ViviDecompilerState *
vivi_decompiler_state_new (guint n_registers)
{
  ViviDecompilerState *state = g_slice_new0 (ViviDecompilerState);

  state->registers = g_slice_alloc0 (sizeof (ViviDecompilerValue) * n_registers);
  state->n_registers = n_registers;
  state->stack = g_array_new (FALSE, TRUE, sizeof (ViviDecompilerValue));

  return state;
}

static void
vivi_decompiler_run (ViviDecompiler *dec)
{
  ViviDecompilerState *state = vivi_decompiler_state_new (4);

  vivi_decompiler_state_free (state);
}

/*** OBJECT ***/

G_DEFINE_TYPE (ViviDecompiler, vivi_decompiler, G_TYPE_OBJECT)

static void
vivi_decompiler_dispose (GObject *object)
{
  ViviDecompiler *dec = VIVI_DECOMPILER (object);
  guint i;

  if (dec->script) {
    swfdec_script_unref (dec->script);
    dec->script = NULL;
  }
  for (i = 0; i < dec->lines->len; i++) {
    g_free (&g_ptr_array_index (dec->lines, i));
  }
  g_ptr_array_free (dec->lines, TRUE);

  G_OBJECT_CLASS (vivi_decompiler_parent_class)->dispose (object);
}

static void
vivi_decompiler_class_init (ViviDecompilerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_decompiler_dispose;
}

static void
vivi_decompiler_init (ViviDecompiler *dec)
{
  dec->lines = g_ptr_array_new ();
}

ViviDecompiler *
vivi_decompiler_new (SwfdecScript *script)
{
  ViviDecompiler *dec = g_object_new (VIVI_TYPE_DECOMPILER, NULL);

  dec->script = swfdec_script_ref (script);
  vivi_decompiler_run (dec);

  return dec;
}

guint
vivi_decompiler_get_n_lines (ViviDecompiler *dec)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER (dec), 0);

  return dec->lines->len;
}

const char *
vivi_decompiler_get_line (ViviDecompiler *dec, guint i)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER (dec), NULL);
  g_return_val_if_fail (i < dec->lines->len, NULL);

  return (const char *) &g_ptr_array_index (dec->lines, i);
}


