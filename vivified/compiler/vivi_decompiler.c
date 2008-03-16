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

static const char * 
vivi_decompiler_value_get (const ViviDecompilerValue *value)
{
  return value->value ? value->value : "undefined";
}

/* NB: takes ownership of string */
static void
vivi_decompiler_value_set (ViviDecompilerValue *value, char *new_string)
{
  g_free (value->value);
  value->value = new_string;
}

typedef struct _ViviDecompilerState ViviDecompilerState;
struct _ViviDecompilerState {
  SwfdecScript *	script;
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
  swfdec_script_unref (state->script);
  g_slice_free (ViviDecompilerState, state);
}

static ViviDecompilerState *
vivi_decompiler_state_new (SwfdecScript *script, const guint8 *pc, guint n_registers)
{
  ViviDecompilerState *state = g_slice_new0 (ViviDecompilerState);

  state->script = swfdec_script_ref (script);
  state->pc = pc;
  state->registers = g_slice_alloc0 (sizeof (ViviDecompilerValue) * n_registers);
  state->n_registers = n_registers;
  state->stack = g_array_new (FALSE, TRUE, sizeof (ViviDecompilerValue));

  return state;
}

static void
vivi_decompiler_state_push (ViviDecompilerState *state, ViviDecompilerValue *val)
{
  g_array_append_val (state->stack, *val);
}

static const ViviDecompilerValue undefined = { NULL, FALSE };

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

static ViviDecompilerState *
vivi_decompiler_state_copy (const ViviDecompilerState *src)
{
  ViviDecompilerState *dest;
  guint i;

  dest = vivi_decompiler_state_new (src->script, src->pc, src->n_registers);
  for (i = 0; i < src->n_registers; i++) {
    if (src->registers[i].value)
      vivi_decompiler_value_copy (&dest->registers[i], &src->registers[i]);
  }
  for (i = 0; i < src->stack->len; i++) {
    ViviDecompilerValue val = { NULL, };
    vivi_decompiler_value_copy (&val, 
	&g_array_index (src->stack, ViviDecompilerValue, i));
    vivi_decompiler_state_push (dest, &val);
  }
  if (src->pool)
    dest->pool = swfdec_constant_pool_copy (src->pool);

  return dest;
}

static const ViviDecompilerValue *
vivi_decompiler_state_get_register (ViviDecompilerState *state, guint reg)
{
  if (reg >= state->n_registers)
    return &undefined;
  else
    return &state->registers[state->n_registers];
}

/*** BLOCK ***/

typedef struct _ViviDecompilerBlock ViviDecompilerBlock;
struct _ViviDecompilerBlock {
  ViviDecompilerState *	start;		/* starting state */
  GPtrArray *		lines;		/* lines parsed from this block */
  ViviDecompilerBlock *	next;		/* block following this one or NULL if returning */
  ViviDecompilerBlock *	branch;		/* NULL or block branched to i if statement */
  /* parsing state */
  guint			incoming;	/* number of incoming blocks */
  const guint8 *	exitpc;		/* pointer to after last parsed command or NULL if not parsed yet */
  char *		label;		/* label generated for this block, so we can goto it */
};

static void
vivi_decompiler_block_unparse (ViviDecompilerBlock *block)
{
  guint i;

  for (i = 0; i < block->lines->len; i++) {
    g_free (g_ptr_array_index (block->lines, i));
  }
  g_ptr_array_set_size (block->lines, 0);
  block->next = NULL;
  block->branch = NULL;
  block->exitpc = NULL;
}

static void
vivi_decompiler_block_free (ViviDecompilerBlock *block)
{
  vivi_decompiler_block_unparse (block);
  vivi_decompiler_state_free (block->start);
  g_ptr_array_free (block->lines, TRUE);
  g_free (block->label);
  g_slice_free (ViviDecompilerBlock, block);
}

static ViviDecompilerBlock *
vivi_decompiler_block_new (ViviDecompilerState *state)
{
  ViviDecompilerBlock *block;

  block = g_slice_new0 (ViviDecompilerBlock);
  block->start = state;
  block->lines = g_ptr_array_new ();

  return block;
}

static const char *
vivi_decompiler_block_get_label (ViviDecompilerBlock *block)
{
  /* NB: lael must be unique */
  if (block->label == NULL)
    block->label = g_strdup_printf ("label_%p", block->start->pc);

  return block->label;
}

static void
vivi_decompiler_block_emit_error (ViviDecompilerBlock *block, ViviDecompilerState *state,
    const char *format, ...) G_GNUC_PRINTF (3, 4);
static void
vivi_decompiler_block_emit_error (ViviDecompilerBlock *block, ViviDecompilerState *state,
    const char *format, ...)
{
  va_list varargs;
  char *s, *t;

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);
  t = g_strdup_printf ("/* %s */", s);
  g_free (s);

  g_ptr_array_add (block->lines, t);
}

static void
vivi_decompiler_block_emit_line (ViviDecompilerBlock *block, ViviDecompilerState *state,
    const char *format, ...) G_GNUC_PRINTF (3, 4);
static void
vivi_decompiler_block_emit_line (ViviDecompilerBlock *block, ViviDecompilerState *state,
    const char *format, ...)
{
  va_list varargs;
  char *s;

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);

  g_ptr_array_add (block->lines, s);
}

static ViviDecompilerBlock *
vivi_decompiler_push_block_for_state (ViviDecompiler *dec, ViviDecompilerState *state)
{
  ViviDecompilerBlock *block;
  GList *walk;

  for (walk = dec->blocks; walk; walk = walk->next) {
    block = walk->data;
    if (block->start->pc < state->pc)
      continue;
    if (block->exitpc && block->exitpc > state->pc) {
      vivi_decompiler_block_unparse (block);
      break;
    }
    if (block->start->pc == state->pc) {
      g_printerr ("FIXME: check that the blocks are equal\n");
      block->incoming++;
      vivi_decompiler_state_free (state);
      return block;
    }
    break;
  }

  /* FIXME: see if the block is already there! */
  block = vivi_decompiler_block_new (state);
  block->incoming++;
  dec->blocks = g_list_insert_before (dec->blocks, walk, block);
  return block;
}

/*** BYTECODE DECOMPILER ***/

typedef gboolean (* DecompileFunc) (ViviDecompilerBlock *block, ViviDecompilerState *state,
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
vivi_decompile_push (ViviDecompilerBlock *block, ViviDecompilerState *state,
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
	  char *s = swfdec_bits_get_string (&bits, state->script->version);
	  if (s == NULL) {
	    vivi_decompiler_block_emit_error (block, state, "could not read string");
	    return FALSE;
	  }
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
	    vivi_decompiler_block_emit_error (block, state, "no constant pool to push from");
	    return FALSE;
	  }
	  if (i >= swfdec_constant_pool_size (state->pool)) {
	    vivi_decompiler_block_emit_error (block, state, "constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (state->pool));
	    return FALSE;
	  }
	  val.value = escape_string (swfdec_constant_pool_get (state->pool, i));
	  break;
	}
      default:
	vivi_decompiler_block_emit_error (block, state, "Push: type %u not implemented", type);
	return FALSE;
    }
    vivi_decompiler_state_push (state, &val);
  }

  state->pc = data + len;
  return TRUE;
}

static gboolean
vivi_decompile_pop (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue val;
  
  vivi_decompiler_state_pop (state, &val);
  state->pc++;
  vivi_decompiler_block_emit_line (block, state, "%s;", vivi_decompiler_value_get (&val));
  vivi_decompiler_value_reset (&val);
  return TRUE;
}

static gboolean
vivi_decompile_constant_pool (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  if (state->pool)
    swfdec_constant_pool_free (state->pool);

  state->pool = swfdec_constant_pool_new_from_action (data, len, state->script->version);
  state->pc = data + len;
  return TRUE;
}

static gboolean
vivi_decompile_trace (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue val;
  
  vivi_decompiler_state_pop (state, &val);
  state->pc++;
  vivi_decompiler_block_emit_line (block, state, "trace (%s);", vivi_decompiler_value_get (&val));
  vivi_decompiler_value_reset (&val);
  return TRUE;
}

static gboolean
vivi_decompile_end (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  return FALSE;
}

static gboolean
vivi_decompile_get_url2 (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue url, target;
  const char *fun;

  vivi_decompiler_state_pop (state, &target);
  vivi_decompiler_state_pop (state, &url);

  state->pc = data + len;
  if (len != 1) {
    vivi_decompiler_block_emit_error (block, state, "invalid length in getURL2 command");   
  } else {
    guint method = data[0] & 3;
    guint internal = data[0] & 64;
    guint variables = data[0] & 128;
    if (method == 3) {
      vivi_decompiler_block_emit_error (block, state, "GetURL method 3 invalid");
      method = 0;
    }

    if (variables) {
      fun = "loadVariables";
    } else if (internal) {
      fun = "loadMovie";
    } else {
      fun = "getURL";
    }
    if (method == 0) {
      vivi_decompiler_block_emit_line (block, state, "%s (%s, %s);", fun, 
	  vivi_decompiler_value_get (&url),
	  vivi_decompiler_value_get (&target));
    } else {
      vivi_decompiler_block_emit_line (block, state, "%s (%s, %s, %s);", fun,
	  vivi_decompiler_value_get (&url),
	  vivi_decompiler_value_get (&target),
	  method == 1 ? "\"GET\"" : "\"POST\"");
    }
  }
  vivi_decompiler_value_reset (&url);
  vivi_decompiler_value_reset (&target);
  return TRUE;
}

static gboolean
vivi_decompile_not (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue val;

  vivi_decompiler_state_pop (state, &val);
  vivi_decompiler_value_set (&val, g_strdup_printf ("!(%s)", vivi_decompiler_value_get (&val)));
  vivi_decompiler_state_push (state, &val);
  state->pc++;
  return TRUE;
}

static DecompileFunc decompile_funcs[256] = {
  [SWFDEC_AS_ACTION_END] = vivi_decompile_end,
  [SWFDEC_AS_ACTION_NOT] = vivi_decompile_not,
  [SWFDEC_AS_ACTION_TRACE] = vivi_decompile_trace,
  [SWFDEC_AS_ACTION_POP] = vivi_decompile_pop,

  [SWFDEC_AS_ACTION_PUSH] = vivi_decompile_push,
  [SWFDEC_AS_ACTION_CONSTANT_POOL] = vivi_decompile_constant_pool,
  [SWFDEC_AS_ACTION_JUMP] = NULL, /* handled directly */
  [SWFDEC_AS_ACTION_GET_URL2] = vivi_decompile_get_url2,
  [SWFDEC_AS_ACTION_IF] = NULL, /* handled directly */
};

static gboolean
vivi_decompiler_process (ViviDecompiler *dec, ViviDecompilerBlock *block, 
    ViviDecompilerState *state, guint code, const guint8 *data, guint len)
{
  switch (code) {
    case SWFDEC_AS_ACTION_IF:
      {
	ViviDecompilerValue val;
	ViviDecompilerState *new;
	gint16 offset;
	char *s, *t;

	if (len != 2) {
	  vivi_decompiler_block_emit_error (block, state, "If action length invalid (is %u, should be 2)", len);
	  return FALSE;
	}
	offset = data[0] | (data[1] << 8);
	state->pc += 5;
	vivi_decompiler_state_pop (state, &val);
	new = vivi_decompiler_state_copy (state);
	block->next = vivi_decompiler_push_block_for_state (dec, new);
	new = vivi_decompiler_state_copy (state);
	new->pc += offset;
	block->branch = vivi_decompiler_push_block_for_state (dec, new);
	s = t = val.value;
	val.value = NULL;
	vivi_decompiler_value_reset (&val);
	len = strlen (s);
	while (*s == '!') {
	  ViviDecompilerBlock *tmp;
	  tmp = block->next;
	  block->next = block->branch;
	  block->branch = tmp;
	  s++;
	  len--;
	  if (s[0] == '(' && s[len - 1] == ')') {
	    s++;
	    len -= 2;
	    s[len] = '\0';
	  }
	}
	vivi_decompiler_block_emit_line (block, state, "if (%s)", s);
	g_free (t);
	return FALSE;
      }
    case SWFDEC_AS_ACTION_JUMP:
      {
	ViviDecompilerState *new;
	gint16 offset;

	if (len != 2) {
	  vivi_decompiler_block_emit_error (block, state, "Jump action length invalid (is %u, should be 2)", len);
	  return FALSE;
	}
	offset = data[0] | (data[1] << 8);
	state->pc += 5;
	new = vivi_decompiler_state_copy (state);
	new->pc += offset;
	block->next = vivi_decompiler_push_block_for_state (dec, new);
	return FALSE;
      }
    default:
      if (decompile_funcs[code]) {
	return decompile_funcs[code] (block, state, code, data, len);
      } else {
	vivi_decompiler_block_emit_error (block, state, "unknown bytecode 0x%02X %u", code, code);
	if (data)
	  state->pc = data + len;
	else
	  state->pc++;
	break;
      }
  };

  return TRUE;
}

static void
vivi_decompiler_block_decompile (ViviDecompiler *dec, ViviDecompilerBlock *block)
{
  ViviDecompilerState *state;
  ViviDecompilerBlock *next_block;
  GList *list;
  const guint8 *start, *end, *exit;
  const guint8 *data;
  guint code, len;

  start = dec->script->buffer->data;
  end = start + dec->script->buffer->length;
  state = vivi_decompiler_state_copy (block->start);
  exit = dec->script->exit;
  list = g_list_find (dec->blocks, block);
  if (list->next) {
    next_block = list->next->data;
    exit = next_block->start->pc;
  } else {
    next_block = NULL;
  }

  while (state->pc != exit) {
    code = state->pc[0];
    if (code & 0x80) {
      if (state->pc + 2 >= end) {
	vivi_decompiler_block_emit_error (block, state, "bytecode %u length value out of range", code);
	goto error;
      }
      data = state->pc + 3;
      len = state->pc[1] | state->pc[2] << 8;
      if (data + len > end) {
	vivi_decompiler_block_emit_error (block, state, "bytecode %u length %u out of range", code, len);
	goto error;
      }
    } else {
      data = NULL;
      len = 0;
    }
    if (!vivi_decompiler_process (dec, block, state, code, data, len))
      goto out;
    if (state->pc < start || state->pc >= end) {
      vivi_decompiler_block_emit_error (block, state, "program counter out of range");
      goto error;
    }
  }
  if (next_block) {
    block->next = next_block;
    next_block->incoming++;
  }

out:
error:
  block->exitpc = state->pc;
  vivi_decompiler_state_free (state);
}

/*** PROGRAM STRUCTURE ANALYSIS ***/

static void
vivi_decompiler_block_append_block (ViviDecompilerBlock *block, 
    ViviDecompilerBlock *append, gboolean indent)
{
  guint i;

  if (indent && append->lines->len > 1)
    vivi_decompiler_block_emit_line (block, NULL, "{");
  for (i = 0; i < append->lines->len; i++) {
    vivi_decompiler_block_emit_line (block, NULL, "%s%s", indent ? "  " : "",
	(char *) g_ptr_array_index (append->lines, i));
  }
  if (indent && append->lines->len > 1)
    vivi_decompiler_block_emit_line (block, NULL, "}");
}

static ViviDecompilerBlock *
vivi_decompiler_find_start_block (ViviDecompiler *dec)
{
  GList *walk;
  
  for (walk = dec->blocks; walk; walk = walk->next) {
    ViviDecompilerBlock *block = walk->data;

    if (block->start->pc == dec->script->main)
      return block;
  }
  g_assert_not_reached ();
  return NULL;
}

static void
vivi_decompiler_merge_blocks_last_resort (ViviDecompiler *dec)
{
  ViviDecompilerBlock *block, *current, *next;
  GList *all_blocks;

  current = vivi_decompiler_find_start_block (dec);
  block = vivi_decompiler_block_new (vivi_decompiler_state_copy (current->start));

  all_blocks = g_list_copy (dec->blocks);
  while (current) {
    g_assert (g_list_find (dec->blocks, current));
    dec->blocks = g_list_remove (dec->blocks, current);
    if (current->incoming) {
      vivi_decompiler_block_emit_line (block, NULL, "%s:", 
	  vivi_decompiler_block_get_label (current));
    }
    vivi_decompiler_block_append_block (block, current, FALSE);
    if (current->branch) {
      vivi_decompiler_block_emit_line (block, NULL, "  goto %s;",
	  vivi_decompiler_block_get_label (current->branch));
    }
    next = current->next;
    if (next == NULL || !g_list_find (dec->blocks, next))
      next = dec->blocks ? dec->blocks->data : NULL;
    if (current->next == NULL) {
      if (next)
	vivi_decompiler_block_emit_line (block, NULL, "return;");
    } else {
      if (next == current->next) {
	next->incoming--;
      } else {
	vivi_decompiler_block_emit_line (block, NULL, "goto %s;", 
	    vivi_decompiler_block_get_label (current->next));
      }
    }
    current = next;
  }
  g_list_foreach (all_blocks, (GFunc) vivi_decompiler_block_free, NULL);
  g_list_free (all_blocks);
  g_assert (dec->blocks == NULL);
  dec->blocks = g_list_prepend (dec->blocks, block);
}

static void
vivi_decompiler_purge_block (ViviDecompiler *dec, ViviDecompilerBlock *block)
{
  g_assert (block->incoming == 0);
  dec->blocks = g_list_remove (dec->blocks, block);
  if (block->next)
    block->next->incoming--;
  if (block->branch)
    block->branch->incoming--;
  vivi_decompiler_block_free (block);
}

static gboolean
vivi_decompiler_merge_if (ViviDecompiler *dec)
{
  ViviDecompilerBlock *block, *if_block, *else_block;
  gboolean result;
  GList *walk;

  result = FALSE;
  for (walk = dec->blocks; walk; walk = walk->next) {
    block = walk->data;
    /* not an if block */
    if (block->branch == NULL)
      continue;
    else_block = block->next;
    if_block = block->branch;
    /* if in if in if in if... */
    if (else_block->branch || if_block->branch)
      continue;
    /* one of the blocks doesn't exist */
    if (if_block == else_block->next) {
      if_block->incoming--;
      if_block = NULL;
    }
    else if (else_block == if_block->next) {
      else_block->incoming--;
      else_block = NULL;
    }
    /* if other blocks reference the blocks, bail, there's loops involved */
    if ((else_block && else_block->incoming > 1) ||
	(if_block && if_block->incoming > 1))
      continue;
    /* if both blocks exist, they must have the same exit block */
    if (if_block && else_block && if_block->next != else_block->next)
      continue;

    /* FINALLY we can merge the blocks */
    block->branch = NULL;
    if (if_block) {
      vivi_decompiler_block_append_block (block, if_block, TRUE);
      block->next = if_block->next;
      if (block->next)
	block->next->incoming++;
      if_block->incoming--;
      vivi_decompiler_purge_block (dec, if_block);
    } else {
      vivi_decompiler_block_emit_line (block, NULL, "  ;");
      block->next = NULL;
    }
    if (else_block) {
      vivi_decompiler_block_emit_line (block, NULL, "else");
      vivi_decompiler_block_append_block (block, else_block, TRUE);
      if (block->next == NULL) {
	block->next = else_block->next;
	if (block->next)
	  block->next->incoming++;
      }
      else_block->incoming--;
      vivi_decompiler_purge_block (dec, else_block);
    }
    result = TRUE;
  }

  return result;
}

static void
vivi_decompiler_merge_blocks (ViviDecompiler *dec)
{
  gboolean restart;

  do {
    restart = FALSE;

    restart |= vivi_decompiler_merge_if (dec);
  } while (restart);
  vivi_decompiler_merge_blocks_last_resort (dec);
}

static void
vivi_decompiler_run (ViviDecompiler *dec)
{
  ViviDecompilerBlock *block;
  ViviDecompilerState *state;
  GList *walk;

  state = vivi_decompiler_state_new (dec->script, dec->script->main, 4);
  if (dec->script->constant_pool) {
    state->pool = swfdec_constant_pool_new_from_action (dec->script->constant_pool->data,
	dec->script->constant_pool->length, dec->script->version);
  }
  block = vivi_decompiler_block_new (state);
  state = vivi_decompiler_state_copy (state);
  g_assert (dec->blocks == NULL);
  dec->blocks = g_list_prepend (dec->blocks, block);
  while (TRUE) {
    for (walk = dec->blocks; walk; walk = walk->next) {
      block = walk->data;
      if (block->exitpc == NULL)
	break;
    }
    if (walk == NULL)
      break;
    vivi_decompiler_block_decompile (dec, block);
  }

  vivi_decompiler_merge_blocks (dec);
}

/*** OBJECT ***/

G_DEFINE_TYPE (ViviDecompiler, vivi_decompiler, G_TYPE_OBJECT)

static void
vivi_decompiler_dispose (GObject *object)
{
  ViviDecompiler *dec = VIVI_DECOMPILER (object);

  if (dec->script) {
    swfdec_script_unref (dec->script);
    dec->script = NULL;
  }
  g_list_foreach (dec->blocks, (GFunc) vivi_decompiler_block_free, NULL);
  g_list_free (dec->blocks);
  dec->blocks = NULL;

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
  ViviDecompilerBlock *block;

  g_return_val_if_fail (VIVI_IS_DECOMPILER (dec), 0);

  if (dec->blocks == NULL)
    return 0;

  block = dec->blocks->data;
  return block->lines->len;
}

const char *
vivi_decompiler_get_line (ViviDecompiler *dec, guint i)
{
  ViviDecompilerBlock *block;

  g_return_val_if_fail (VIVI_IS_DECOMPILER (dec), NULL);
  g_return_val_if_fail (dec->blocks != NULL, NULL);
  block = dec->blocks->data;
  g_return_val_if_fail (i < block->lines->len, NULL);

  return g_ptr_array_index (block->lines, i);
}


