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
#include "vivi_decompiler_block.h"
#include "vivi_decompiler_state.h"
#include "vivi_decompiler_value.h"

#if 0
static G_GNUC_UNUSED void
DUMP_BLOCKS (ViviDecompiler *dec)
{
  GList *walk;

  g_print ("dumping blocks:\n");
  for (walk = dec->blocks; walk; walk = walk->next) {
    ViviDecompilerBlock *block = walk->data;
    g_print ("  %p -> %p\n", block->start->pc, block->exitpc);
  }
}
#else
#define DUMP_BLOCKS(dec) (void) 0
#endif

static ViviDecompilerBlock *
vivi_decompiler_push_block_for_state (ViviDecompiler *dec, ViviDecompilerState *state)
{
  ViviDecompilerBlock *block;
  GList *walk;
  const guint8 *pc, *block_start;

  DUMP_BLOCKS (dec);
  pc = vivi_decompiler_state_get_pc (state);
  for (walk = dec->blocks; walk; walk = walk->next) {
    block = walk->data;
    block_start = vivi_decompiler_block_get_start (block);
    if (block_start < pc) {
      if (vivi_decompiler_block_contains (block, pc)) {
	vivi_decompiler_block_reset (block);
	break;
      }
      continue;
    }
    if (block_start == pc) {
      g_printerr ("FIXME: check that the blocks are equal\n");
      vivi_decompiler_state_free (state);
      return block;
    }
    break;
  }

  /* FIXME: see if the block is already there! */
  block = vivi_decompiler_block_new (state);
  dec->blocks = g_list_insert_before (dec->blocks, walk ? walk->next : NULL, block);
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
  ViviDecompilerValue *val;
  SwfdecBits bits;
  guint type;
  char *value;

  swfdec_bits_init_data (&bits, data, len);
  while (swfdec_bits_left (&bits)) {
    type = swfdec_bits_get_u8 (&bits);
    switch (type) {
      case 0: /* string */
	{
	  char *s = swfdec_bits_get_string (&bits, vivi_decompiler_state_get_version (state));
	  if (s == NULL) {
	    vivi_decompiler_block_emit_error (block, "could not read string");
	    return FALSE;
	  }
	  value = escape_string (s);
	  g_free (s);
	  break;
	}
      case 1: /* float */
	value = g_strdup_printf ("%f", swfdec_bits_get_float (&bits));
	break;
      case 2: /* null */
	value = g_strdup ("null");
	break;
      case 3: /* undefined */
	break;
      case 4: /* register */
	{
	  val = vivi_decompiler_value_copy (vivi_decompiler_state_get_register (
		state, swfdec_bits_get_u8 (&bits)));
	  vivi_decompiler_state_push (state, val);
	  continue;
	}
      case 5: /* boolean */
	value = g_strdup (swfdec_bits_get_u8 (&bits) ? "true" : "false");
	break;
      case 6: /* double */
	value = g_strdup_printf ("%g", swfdec_bits_get_double (&bits));
	break;
      case 7: /* 32bit int */
	value = g_strdup_printf ("%d", swfdec_bits_get_u32 (&bits));
	break;
      case 8: /* 8bit ConstantPool address */
      case 9: /* 16bit ConstantPool address */
	{
	  guint i = type == 8 ? swfdec_bits_get_u8 (&bits) : swfdec_bits_get_u16 (&bits);
	  const SwfdecConstantPool *pool = vivi_decompiler_state_get_constant_pool (state);
	  if (pool == NULL) {
	    vivi_decompiler_block_emit_error (block, "no constant pool to push from");
	    return FALSE;
	  }
	  if (i >= swfdec_constant_pool_size (pool)) {
	    vivi_decompiler_block_emit_error (block, "constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (pool));
	    return FALSE;
	  }
	  value = escape_string (swfdec_constant_pool_get (pool, i));
	  break;
	}
      default:
	vivi_decompiler_block_emit_error (block, "Push: type %u not implemented", type);
	return FALSE;
    }
    val = vivi_decompiler_value_new (VIVI_PRECEDENCE_NONE, TRUE, value);
    vivi_decompiler_state_push (state, val);
  }

  return TRUE;
}

static gboolean
vivi_decompile_pop (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue *val;
  
  val = vivi_decompiler_state_pop (state);
  vivi_decompiler_block_emit (block, state, "%s;", vivi_decompiler_value_get_text (val));
  vivi_decompiler_value_free (val);
  return TRUE;
}

static gboolean
vivi_decompile_constant_pool (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  SwfdecConstantPool *pool = swfdec_constant_pool_new_from_action (data, len, 
      vivi_decompiler_state_get_version (state));

  vivi_decompiler_state_set_constant_pool (state, pool);
  return TRUE;
}

static gboolean
vivi_decompile_trace (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue *val;
  
  val = vivi_decompiler_state_pop (state);
  vivi_decompiler_block_emit (block, state, "trace (%s);", vivi_decompiler_value_get_text (val));
  vivi_decompiler_value_free (val);
  return TRUE;
}

static gboolean
vivi_decompile_end (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  vivi_decompiler_block_finish (block, state);
  return FALSE;
}

static gboolean
vivi_decompile_get_url2 (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue *url, *target;
  const char *fun;

  target = vivi_decompiler_state_pop (state);
  url = vivi_decompiler_state_pop (state);

  if (len != 1) {
    vivi_decompiler_block_emit_error (block, "invalid length in getURL2 command");   
  } else {
    guint method = data[0] & 3;
    guint internal = data[0] & 64;
    guint variables = data[0] & 128;
    if (method == 3) {
      vivi_decompiler_block_emit_error (block, "GetURL method 3 invalid");
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
      vivi_decompiler_block_emit (block, state, "%s (%s, %s);", fun, 
	  vivi_decompiler_value_get_text (url),
	  vivi_decompiler_value_get_text (target));
    } else {
      vivi_decompiler_block_emit (block, state, "%s (%s, %s, %s);", fun,
	  vivi_decompiler_value_get_text (url),
	  vivi_decompiler_value_get_text (target),
	  method == 1 ? "\"GET\"" : "\"POST\"");
    }
  }
  vivi_decompiler_value_free (url);
  vivi_decompiler_value_free (target);
  return TRUE;
}

static gboolean
vivi_decompile_not (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviDecompilerValue *pop, *push;

  pop = vivi_decompiler_state_pop (state);
  pop = vivi_decompiler_value_ensure_precedence (pop, VIVI_PRECEDENCE_UNARY);
  push = vivi_decompiler_value_new_printf (VIVI_PRECEDENCE_UNARY,
      vivi_decompiler_value_is_constant (pop), "!%s", vivi_decompiler_value_get_text (pop));
  vivi_decompiler_state_push (state, push);
  vivi_decompiler_value_free (pop);
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
  gboolean result;

  switch (code) {
    case SWFDEC_AS_ACTION_IF:
      {
	ViviDecompilerBlock *next, *branch;
	ViviDecompilerValue *val, *val2;
	ViviDecompilerState *new;
	gint16 offset;

	if (len != 2) {
	  vivi_decompiler_block_emit_error (block, "If action length invalid (is %u, should be 2)", len);
	  return FALSE;
	}
	offset = data[0] | (data[1] << 8);
	vivi_decompiler_state_add_pc (state, 5);
	val = vivi_decompiler_state_pop (state);
	vivi_decompiler_block_finish (block, state);
	new = vivi_decompiler_state_copy (state);
	next = vivi_decompiler_push_block_for_state (dec, new);
	new = vivi_decompiler_state_copy (state);
	vivi_decompiler_state_add_pc (new, offset);
	branch = vivi_decompiler_push_block_for_state (dec, new);
	if (vivi_decompiler_block_is_finished (block)) {
	  char *s, *t;
	  ViviPrecedence prec = vivi_decompiler_value_get_precedence (val);
	  s = t = g_strdup (vivi_decompiler_value_get_text (val));
	  len = strlen (s);
	  while (*s == '!') {
	    ViviDecompilerBlock *tmp;
	    tmp = next;
	    next = branch;
	    branch = tmp;
	    s++;
	    len--;
	    if (s[0] == '(' && s[len - 1] == ')') {
	      s++;
	      len -= 2;
	      s[len] = '\0';
	    }
	    prec = VIVI_PRECEDENCE_COMMA;
	  }
	  val2 = vivi_decompiler_value_new (prec, vivi_decompiler_value_is_constant (val),
	      g_strdup (s));
	  vivi_decompiler_value_free (val);
	  vivi_decompiler_block_set_next (block, next);
	  vivi_decompiler_block_set_branch (block, branch, val2);
	  g_free (t);
	}
      }
      return FALSE;
    case SWFDEC_AS_ACTION_JUMP:
      {
	ViviDecompilerBlock *next;
	ViviDecompilerState *new;
	gint16 offset;

	if (len != 2) {
	  vivi_decompiler_block_emit_error (block, "Jump action length invalid (is %u, should be 2)", len);
	  return FALSE;
	}
	offset = data[0] | (data[1] << 8);
	vivi_decompiler_state_add_pc (state, 5);
	vivi_decompiler_block_finish (block, state);
	new = vivi_decompiler_state_copy (state);
	vivi_decompiler_state_add_pc (new, offset);
	next = vivi_decompiler_push_block_for_state (dec, new);
	if (vivi_decompiler_block_is_finished (block)) {
	  vivi_decompiler_block_set_next (block, next);
	}
      }
      return FALSE;
    default:
      if (decompile_funcs[code]) {
	result = decompile_funcs[code] (block, state, code, data, len);
      } else {
	vivi_decompiler_block_emit_error (block, "unknown bytecode 0x%02X %u", code, code);
	result = FALSE;
      }
      if (data)
	vivi_decompiler_state_add_pc (state, 3 + len);
      else
	vivi_decompiler_state_add_pc (state, 1);
      return result;
  };

  return TRUE;
}

static void
vivi_decompiler_block_decompile (ViviDecompiler *dec, ViviDecompilerBlock *block)
{
  ViviDecompilerState *state;
  ViviDecompilerBlock *next_block;
  GList *list;
  const guint8 *pc, *start, *end, *exit;
  const guint8 *data;
  guint code, len;

  start = dec->script->buffer->data;
  end = start + dec->script->buffer->length;
  state = vivi_decompiler_state_copy (vivi_decompiler_block_get_start_state (block));
  exit = dec->script->exit;
  list = g_list_find (dec->blocks, block);
  if (list->next) {
    next_block = list->next->data;
    exit = vivi_decompiler_block_get_start (next_block);
  } else {
    next_block = NULL;
  }

  while ((pc = vivi_decompiler_state_get_pc (state)) != exit) {
    if (pc < start || pc >= end) {
      vivi_decompiler_block_emit_error (block, "program counter out of range");
      goto error;
    }
    code = pc[0];
    if (code & 0x80) {
      if (pc + 2 >= end) {
	vivi_decompiler_block_emit_error (block, "bytecode %u length value out of range", code);
	goto error;
      }
      data = pc + 3;
      len = pc[1] | pc[2] << 8;
      if (data + len > end) {
	vivi_decompiler_block_emit_error (block, "bytecode %u length %u out of range", code, len);
	goto error;
      }
    } else {
      data = NULL;
      len = 0;
    }
    if (!vivi_decompiler_process (dec, block, state, code, data, len))
      goto out;
  }
  vivi_decompiler_block_finish (block, state);
  if (next_block) {
    vivi_decompiler_block_set_next (block, next_block);
  }

out:
error:
  vivi_decompiler_state_free (state);
}

/*** PROGRAM STRUCTURE ANALYSIS ***/

static void
vivi_decompiler_block_append_block (ViviDecompilerBlock *block, 
    ViviDecompilerBlock *append, gboolean indent)
{
  guint i, lines;

  lines = vivi_decompiler_block_get_n_lines (append);
  if (indent && lines > 1)
    vivi_decompiler_block_emit (block, NULL, "{");
  for (i = 0; i < lines; i++) {
    vivi_decompiler_block_emit (block, NULL, "%s%s", indent ? "  " : "",
	vivi_decompiler_block_get_line (append, i));
  }
  if (indent && lines > 1)
    vivi_decompiler_block_emit (block, NULL, "}");
}

static ViviDecompilerBlock *
vivi_decompiler_find_start_block (ViviDecompiler *dec)
{
  GList *walk;
  
  for (walk = dec->blocks; walk; walk = walk->next) {
    ViviDecompilerBlock *block = walk->data;

    if (vivi_decompiler_block_get_start (block) == dec->script->main)
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
  guint max_incoming = 0;

  current = vivi_decompiler_find_start_block (dec);
  block = vivi_decompiler_block_new (vivi_decompiler_state_copy (
	vivi_decompiler_block_get_start_state (current)));

  all_blocks = g_list_copy (dec->blocks);
  while (current) {
    g_assert (g_list_find (dec->blocks, current));
    dec->blocks = g_list_remove (dec->blocks, current);
    if (vivi_decompiler_block_get_n_incoming (current) > max_incoming) {
      vivi_decompiler_block_force_label (current);
      vivi_decompiler_block_emit (block, NULL, "%s:", 
	  vivi_decompiler_block_get_label (current));
    }
    max_incoming = 0;
    vivi_decompiler_block_append_block (block, current, FALSE);
    next = vivi_decompiler_block_get_branch (current);
    if (next) {
      vivi_decompiler_block_force_label (next);
      vivi_decompiler_block_emit (block, NULL, "if (%s)", vivi_decompiler_value_get_text (
	  vivi_decompiler_block_get_branch_condition (next)));
      vivi_decompiler_block_emit (block, NULL, "  goto %s;",
	  vivi_decompiler_block_get_label (next));
    }
    next = vivi_decompiler_block_get_next (current);
    if (next == NULL || !g_list_find (dec->blocks, next))
      next = dec->blocks ? dec->blocks->data : NULL;
    if (vivi_decompiler_block_get_next (current) == NULL) {
      if (next)
	vivi_decompiler_block_emit (block, NULL, "return;");
    } else {
      if (next == vivi_decompiler_block_get_next (current)) {
	max_incoming = 1;
      } else {
	vivi_decompiler_block_force_label (next);
	vivi_decompiler_block_emit (block, NULL, "goto %s;", 
	    vivi_decompiler_block_get_label (next));
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
  g_assert (vivi_decompiler_block_get_n_incoming (block) == 0);
  dec->blocks = g_list_remove (dec->blocks, block);
  vivi_decompiler_block_free (block);
}

/*  ONE
 *   |              ==>   BLOCK
 *  TWO
 */
static gboolean
vivi_decompiler_merge_lines (ViviDecompiler *dec)
{
  ViviDecompilerBlock *block, *next;
  const ViviDecompilerValue *val;
  gboolean result;
  GList *walk;

  result = FALSE;
  for (walk = dec->blocks; walk; walk = walk->next) {
    block = walk->data;

    /* This is an if block or so */
    if (vivi_decompiler_block_get_branch (block) != NULL)
      continue;
    /* has no next block */
    next = vivi_decompiler_block_get_next (block);
    if (next == NULL)
      continue;
    /* The next block has multiple incoming blocks */
    if (vivi_decompiler_block_get_n_incoming (next) != 1)
      continue;

    vivi_decompiler_block_append_block (block, next, FALSE);
    vivi_decompiler_block_set_next (block, vivi_decompiler_block_get_next (next));
    val = vivi_decompiler_block_get_branch_condition (next);
    if (val) {
      vivi_decompiler_block_set_branch (block, 
	  vivi_decompiler_block_get_branch (next),
	  vivi_decompiler_value_copy (val));
    }
    vivi_decompiler_purge_block (dec, next);
  }

  return result;
}

/*     COND
 *    /    \
 *  [IF] [ELSE]     ==>   BLOCK
 *    \    /
 *     NEXT
 */
static gboolean
vivi_decompiler_merge_if (ViviDecompiler *dec)
{
  ViviDecompilerBlock *block, *if_block, *else_block;
  gboolean result;
  GList *walk;

  result = FALSE;
  for (walk = dec->blocks; walk; walk = walk->next) {
    block = walk->data;

    if_block = vivi_decompiler_block_get_branch (block);
    else_block = vivi_decompiler_block_get_next (block);
    /* not an if block */
    if (if_block == NULL)
      continue;
    /* one of the blocks doesn't exist */
    if (if_block == vivi_decompiler_block_get_next (else_block)) 
      if_block = NULL;
    else if (else_block == vivi_decompiler_block_get_next (if_block))
      else_block = NULL;
    /* if in if in if in if... */
    if ((else_block && vivi_decompiler_block_get_branch (else_block)) || 
	(if_block && vivi_decompiler_block_get_branch (if_block)))
      continue;
    /* if other blocks reference the blocks, bail, there's loops involved */
    if ((else_block && vivi_decompiler_block_get_n_incoming (else_block) > 1) ||
	(if_block && vivi_decompiler_block_get_n_incoming (if_block) > 1))
      continue;
    /* if both blocks exist, they must have the same exit block */
    if (if_block && else_block && 
	vivi_decompiler_block_get_next (if_block) != vivi_decompiler_block_get_next (else_block))
      continue;

    /* FINALLY we can merge the blocks */
    if (if_block) {
      vivi_decompiler_block_emit (block, NULL, "if (%s)", 
	  vivi_decompiler_value_get_text (vivi_decompiler_block_get_branch_condition (block)));
      vivi_decompiler_block_append_block (block, if_block, TRUE);
      vivi_decompiler_block_set_next (block,
	  vivi_decompiler_block_get_next (if_block));
      vivi_decompiler_block_set_branch (block, NULL, NULL);
      vivi_decompiler_purge_block (dec, if_block);
      if (else_block)
	vivi_decompiler_block_emit (block, NULL, "else");
    } else {
      ViviDecompilerValue *cond = vivi_decompiler_value_copy (
	  vivi_decompiler_block_get_branch_condition (block));
      cond = vivi_decompiler_value_ensure_precedence (cond, VIVI_PRECEDENCE_UNARY);
      vivi_decompiler_block_emit (block, NULL, "if (!%s)", vivi_decompiler_value_get_text (cond));
      vivi_decompiler_value_free (cond);
      vivi_decompiler_block_set_branch (block, NULL, NULL);
    }
    if (else_block) {
      vivi_decompiler_block_append_block (block, else_block, TRUE);
      vivi_decompiler_block_set_next (block,
	  vivi_decompiler_block_get_next (else_block));
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

  DUMP_BLOCKS (dec);

  do {
    restart = FALSE;

    restart |= vivi_decompiler_merge_lines (dec);
    restart |= vivi_decompiler_merge_if (dec);
  } while (restart);

  DUMP_BLOCKS (dec);
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
    vivi_decompiler_state_set_constant_pool (state,
	swfdec_constant_pool_new_from_action (dec->script->constant_pool->data,
	    dec->script->constant_pool->length, dec->script->version));
  }
  block = vivi_decompiler_block_new (state);
  g_assert (dec->blocks == NULL);
  dec->blocks = g_list_prepend (dec->blocks, block);
  while (TRUE) {
    for (walk = dec->blocks; walk; walk = walk->next) {
      block = walk->data;
      if (!vivi_decompiler_block_is_finished (block))
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
  g_return_val_if_fail (VIVI_IS_DECOMPILER (dec), 0);

  if (dec->blocks == NULL)
    return 0;

  return vivi_decompiler_block_get_n_lines (dec->blocks->data);
}

const char *
vivi_decompiler_get_line (ViviDecompiler *dec, guint i)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER (dec), NULL);
  g_return_val_if_fail (dec->blocks != NULL, NULL);

  return vivi_decompiler_block_get_line (dec->blocks->data, i);
}

