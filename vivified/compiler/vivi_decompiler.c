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

#include <string.h>

#include <swfdec/swfdec_as_interpret.h>
#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_script_internal.h>

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
  value->unconstant = FALSE;
}

static void
vivi_decompiler_value_copy (ViviDecompilerValue *dest, const ViviDecompilerValue *src)
{
  dest->value = g_strdup (src->value);
  dest->unconstant = src->unconstant;
}

typedef struct _ViviDecompilerState ViviDecompilerState;
struct _ViviDecompilerState {
  const guint8 *	pc;
  ViviDecompilerValue *	registers;
  guint			n_registers;
  GArray *		stack;
  SwfdecConstantPool *	pool;
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

  if (state->pool)
    swfdec_constant_pool_free (state->pool);
  g_array_free (state->stack, TRUE);
  g_slice_free1 (sizeof (ViviDecompilerValue) * state->n_registers, state->registers);
  g_slice_free (ViviDecompilerState, state);
}

static ViviDecompilerState *
vivi_decompiler_state_new (const guint8 *pc, guint n_registers)
{
  ViviDecompilerState *state = g_slice_new0 (ViviDecompilerState);

  state->pc = pc;
  state->registers = g_slice_alloc0 (sizeof (ViviDecompilerValue) * n_registers);
  state->n_registers = n_registers;
  state->stack = g_array_new (FALSE, TRUE, sizeof (ViviDecompilerValue));

  return state;
}

static void
vivi_decompiler_state_push (ViviDecompilerState *state, const ViviDecompilerValue *val)
{
  g_array_append_val (state->stack, *val);
}

static const ViviDecompilerValue undefined = { (char *) "undefined", FALSE };

static void
vivi_decompiler_state_pop (ViviDecompilerState *state, ViviDecompilerValue *val)
{
  if (state->stack->len == 0) {
    if (val)
      *val = undefined;
  } else {
    ViviDecompilerValue *pop;
    pop = &g_array_index (state->stack, ViviDecompilerValue, state->stack->len - 1);
    if (val)
      *val = *pop;
    else
      vivi_decompiler_value_reset (pop);
  }

  g_array_set_size (state->stack, state->stack->len - 1);
}

static const ViviDecompilerValue *
vivi_decompiler_state_get_register (ViviDecompilerState *state, guint reg)
{
  if (reg >= state->n_registers)
    return &undefined;
  else
    return &state->registers[state->n_registers];
}

static void
vivi_decompiler_emit_error (ViviDecompiler *dec, ViviDecompilerState *state,
    const char *format, ...) G_GNUC_PRINTF (3, 4);
static void
vivi_decompiler_emit_error (ViviDecompiler *dec, ViviDecompilerState *state,
    const char *format, ...)
{
  va_list varargs;
  char *s, *t;

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);
  t = g_strdup_printf ("/* %s */", s);
  g_free (s);

  g_ptr_array_add (dec->lines, t);
}

static void
vivi_decompiler_emit_line (ViviDecompiler *dec, ViviDecompilerState *state,
    const char *format, ...) G_GNUC_PRINTF (3, 4);
static void
vivi_decompiler_emit_line (ViviDecompiler *dec, ViviDecompilerState *state,
    const char *format, ...)
{
  va_list varargs;
  char *s;

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);

  g_ptr_array_add (dec->lines, s);
}

/*** BYTECODE DECOMPILER ***/

typedef gboolean (* DecompileFunc) (ViviDecompiler *dec, ViviDecompilerState *state,
          guint code, const guint8 *data, guint len);

static char *
escape_string (const char *s)
{
  GString *str;
  char *next;

  str = g_string_new ("\"");
  while ((next = strpbrk (s, "\"\n"))) {
    g_string_append_len (str, s, next - s);
    switch (*next) {
      case '"':
	g_string_append (str, "\"");
	break;
      case '\n':
	g_string_append (str, "\n");
	break;
      default:
	g_assert_not_reached ();
    }
    s = next + 1;
  }
  g_string_append (str, s);
  g_string_append_c (str, '"');
  return g_string_free (str, FALSE);
}

static gboolean
vivi_decompile_push (ViviDecompiler *dec, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue val;
  SwfdecBits bits;
  guint type;

  swfdec_bits_init_data (&bits, data, len);
  while (swfdec_bits_left (&bits)) {
    memset (&val, 0, sizeof (ViviDecompilerValue));
    type = swfdec_bits_get_u8 (&bits);
    switch (type) {
      case 0: /* string */
	{
	  char *s = swfdec_bits_get_string (&bits, dec->script->version);
	  if (s == NULL)
	    return FALSE;
	  val.value = escape_string (s);
	  g_free (s);
	  break;
	}
      case 1: /* float */
	val.value = g_strdup_printf ("%f", swfdec_bits_get_float (&bits));
	break;
      case 2: /* null */
	val.value = g_strdup ("null");
	break;
      case 3: /* undefined */
	val.value = g_strdup ("undefined");
	break;
      case 4: /* register */
	{
	  vivi_decompiler_value_copy (&val,
	      vivi_decompiler_state_get_register (state, swfdec_bits_get_u8 (&bits)));
	  break;
	}
      case 5: /* boolean */
	val.value = g_strdup (swfdec_bits_get_u8 (&bits) ? "true" : "false");
	break;
      case 6: /* double */
	val.value = g_strdup_printf ("%g", swfdec_bits_get_double (&bits));
	break;
      case 7: /* 32bit int */
	val.value = g_strdup_printf ("%d", swfdec_bits_get_u32 (&bits));
	break;
      case 8: /* 8bit ConstantPool address */
      case 9: /* 16bit ConstantPool address */
	{
	  guint i = type == 8 ? swfdec_bits_get_u8 (&bits) : swfdec_bits_get_u16 (&bits);
	  if (state->pool == NULL) {
	    vivi_decompiler_emit_error (dec, state,"no constant pool to push from");
	    return FALSE;
	  }
	  if (i >= swfdec_constant_pool_size (state->pool)) {
	    vivi_decompiler_emit_error (dec, state,"constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (state->pool));
	    return FALSE;
	  }
	  val.value = escape_string (swfdec_constant_pool_get (state->pool, i));
	  break;
	}
      default:
	vivi_decompiler_emit_error (dec, state,"Push: type %u not implemented", type);
	return FALSE;
    }
    vivi_decompiler_state_push (state, &val);
  }

  state->pc = data + len;
  return TRUE;
}

static gboolean
vivi_decompile_pop (ViviDecompiler *dec, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue val;
  
  vivi_decompiler_state_pop (state, &val);
  state->pc++;
  vivi_decompiler_emit_line (dec, state, "%s;", val.value);
  vivi_decompiler_value_reset (&val);
  return TRUE;
}

static gboolean
vivi_decompile_constant_pool (ViviDecompiler *dec, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  if (state->pool)
    swfdec_constant_pool_free (state->pool);

  state->pool = swfdec_constant_pool_new_from_action (data, len, dec->script->version);
  state->pc = data + len;
  return TRUE;
}

static gboolean
vivi_decompile_trace (ViviDecompiler *dec, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue val;
  
  vivi_decompiler_state_pop (state, &val);
  state->pc++;
  vivi_decompiler_emit_line (dec, state, "trace (%s);", val.value);
  vivi_decompiler_value_reset (&val);
  return TRUE;
}

static gboolean
vivi_decompile_end (ViviDecompiler *dec, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  state->pc = dec->script->exit;
  vivi_decompiler_emit_line (dec, state, "return;");
  return TRUE;
}

static gboolean
vivi_decompile_get_url2 (ViviDecompiler *dec, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue url, target;
  const char *fun;

  vivi_decompiler_state_pop (state, &target);
  vivi_decompiler_state_pop (state, &url);

  state->pc = data + len;
  if (len != 1) {
    vivi_decompiler_emit_error (dec, state, "invalid getURL2 command");   
  } else {
    guint method = data[0] & 3;
    guint internal = data[0] & 64;
    guint variables = data[0] & 128;
    if (method == 3) {
      vivi_decompiler_emit_error (dec, state, "GetURL method 3 invalid");
      method = 0;
    }

    if (variables) {
      fun = "loadVariables";
    } else if (internal) {
      fun = "loadMovie";
    } else {
      fun = "getURL";
    }
    if (method == 0)
      vivi_decompiler_emit_line (dec, state, "%s (%s, %s);", fun, url.value, target.value);
    else
      vivi_decompiler_emit_line (dec, state, "%s (%s, %s, %s);", 
	  fun, url.value, target.value, method == 1 ? "\"GET\"" : "\"POST\"");
  }
  vivi_decompiler_value_reset (&url);
  vivi_decompiler_value_reset (&target);
  return TRUE;
}

static DecompileFunc decompile_funcs[256] = {
  [SWFDEC_AS_ACTION_END] = vivi_decompile_end,
  [SWFDEC_AS_ACTION_TRACE] = vivi_decompile_trace,
  [SWFDEC_AS_ACTION_POP] = vivi_decompile_pop,

  [SWFDEC_AS_ACTION_PUSH] = vivi_decompile_push,
  [SWFDEC_AS_ACTION_CONSTANT_POOL] = vivi_decompile_constant_pool,
  [SWFDEC_AS_ACTION_GET_URL2] = vivi_decompile_get_url2,
};

static gboolean
vivi_decompiler_process (ViviDecompiler *dec, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  switch (code) {
    default:
      if (decompile_funcs[code]) {
	return decompile_funcs[code] (dec, state, code, data, len);
      } else {
	vivi_decompiler_emit_error (dec, state,"unknown bytecode 0x%02X %u", code, code);
	return FALSE;
      }
  };

  return TRUE;
}

static void
vivi_decompiler_run (ViviDecompiler *dec)
{
  ViviDecompilerState *state;
  const guint8 *start, *end;
  const guint8 *data;
  guint code, len;

  start = dec->script->buffer->data;
  end = start + dec->script->buffer->length;
  state = vivi_decompiler_state_new (dec->script->main, 4);
  if (dec->script->constant_pool) {
    state->pool = swfdec_constant_pool_new_from_action (dec->script->constant_pool->data,
	dec->script->constant_pool->length, dec->script->version);
  }

  while (state->pc != dec->script->exit) {
    code = state->pc[0];
    if (code & 0x80) {
      if (state->pc + 2 >= end) {
	vivi_decompiler_emit_error (dec, state,"bytecode %u length value out of range", code);
	goto error;
      }
      data = state->pc + 3;
      len = state->pc[1] | state->pc[2] << 8;
      if (data + len > end) {
	vivi_decompiler_emit_error (dec, state,"bytecode %u length %u out of range", code, len);
	goto error;
      }
    } else {
      data = NULL;
      len = 0;
    }
    if (!vivi_decompiler_process (dec, state, code, data, len))
      goto error;
    if (state->pc < start || state->pc >= end) {
      vivi_decompiler_emit_error (dec, state,"program counter out of range");
      goto error;
    }
  }

error:
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

  return g_ptr_array_index (dec->lines, i);
}


