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
#include "vivi_code_constant.h"
#include "vivi_code_get_url.h"
#include "vivi_code_if.h"
#include "vivi_code_return.h"
#include "vivi_code_trace.h"
#include "vivi_code_unary.h"
#include "vivi_code_value_statement.h"
#include "vivi_decompiler_block.h"
#include "vivi_decompiler_state.h"

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
  ViviCodeValue *val;
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
	    vivi_decompiler_block_add_error (block, "could not read string");
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
	  val = vivi_decompiler_state_get_register (
		state, swfdec_bits_get_u8 (&bits));
	  vivi_decompiler_state_push (state, g_object_ref (val));
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
	    vivi_decompiler_block_add_error (block, "no constant pool to push from");
	    return FALSE;
	  }
	  if (i >= swfdec_constant_pool_size (pool)) {
	    vivi_decompiler_block_add_error (block, "constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (pool));
	    return FALSE;
	  }
	  value = escape_string (swfdec_constant_pool_get (pool, i));
	  break;
	}
      default:
	vivi_decompiler_block_add_error (block, "Push: type %u not implemented", type);
	return FALSE;
    }
    val = VIVI_CODE_VALUE (vivi_code_constant_new (value));
    vivi_decompiler_state_push (state, val);
  }

  return TRUE;
}

static gboolean
vivi_decompile_pop (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeToken *stmt;
  ViviCodeValue *val;
  
  val = vivi_decompiler_state_pop (state);
  stmt = vivi_code_value_statement_new (val);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (stmt));
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
  ViviCodeToken *trace;
  ViviCodeValue *val;
  
  val = vivi_decompiler_state_pop (state);
  trace = vivi_code_trace_new (val);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (trace));
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
  ViviCodeValue *url, *target;

  target = vivi_decompiler_state_pop (state);
  url = vivi_decompiler_state_pop (state);

  if (len != 1) {
    vivi_decompiler_block_add_error (block, "invalid length in getURL2 command");   
    g_object_unref (target);
    g_object_unref (url);
  } else {
    ViviCodeToken *token;
    guint method = data[0] & 3;
    guint internal = data[0] & 64;
    guint variables = data[0] & 128;

    token = vivi_code_get_url_new (target, url, method, internal, variables);
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (token));
  }
  return TRUE;
}

static gboolean
vivi_decompile_not (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *val;

  val = vivi_decompiler_state_pop (state);
  val = VIVI_CODE_VALUE (vivi_code_unary_new (val, '!'));
  vivi_decompiler_state_push (state, val);
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
	ViviCodeValue *val;
	ViviDecompilerState *new;
	gint16 offset;

	if (len != 2) {
	  vivi_decompiler_block_add_error (block, "If action length invalid (is %u, should be 2)", len);
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
	  vivi_decompiler_block_set_next (block, next);
	  vivi_decompiler_block_set_branch (block, branch, val);
	} else {
	  g_object_unref (val);
	}
      }
      return FALSE;
    case SWFDEC_AS_ACTION_JUMP:
      {
	ViviDecompilerBlock *next;
	ViviDecompilerState *new;
	gint16 offset;

	if (len != 2) {
	  vivi_decompiler_block_add_error (block, "Jump action length invalid (is %u, should be 2)", len);
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
	vivi_decompiler_block_add_error (block, "unknown bytecode 0x%02X %u", code, code);
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
      vivi_decompiler_block_add_error (block, "program counter out of range");
      goto error;
    }
    code = pc[0];
    if (code & 0x80) {
      if (pc + 2 >= end) {
	vivi_decompiler_block_add_error (block, "bytecode %u length value out of range", code);
	goto error;
      }
      data = pc + 3;
      len = pc[1] | pc[2] << 8;
      if (data + len > end) {
	vivi_decompiler_block_add_error (block, "bytecode %u length %u out of range", code, len);
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
  ViviCodeBlock *block;
  ViviDecompilerBlock *current, *next;
  GList *ordered, *walk;

  current = vivi_decompiler_find_start_block (dec);

  ordered = NULL;
  while (current) {
    g_assert (g_list_find (dec->blocks, current));
    dec->blocks = g_list_remove (dec->blocks, current);
    ordered = g_list_prepend (ordered, current);
    next = vivi_decompiler_block_get_branch (current);
    if (next)
      vivi_decompiler_block_force_label (next);
    next = vivi_decompiler_block_get_next (current);
    if (next == NULL || !g_list_find (dec->blocks, next)) {
      if (next)
	vivi_decompiler_block_force_label (next);
      next = dec->blocks ? dec->blocks->data : NULL;
    }
    current = next;
  }
  g_assert (dec->blocks == NULL);
  ordered = g_list_reverse (ordered);

  block = VIVI_CODE_BLOCK (vivi_code_block_new ());
  for (walk = ordered; walk; walk = walk->next) {
    current = walk->data;
    next = vivi_decompiler_block_get_next (current);
    if (walk->next && next == walk->next->data)
      vivi_decompiler_block_set_next (current, NULL);
    vivi_decompiler_block_add_to_block (current, block);
    if (next == NULL && walk->next != NULL)
      vivi_code_block_add_statement (block, VIVI_CODE_STATEMENT (vivi_code_return_new ()));
  }
  g_list_foreach (ordered, (GFunc) g_object_unref, NULL);
  g_list_free (ordered);
  dec->blocks = g_list_prepend (dec->blocks, block);
}

static void
vivi_decompiler_purge_block (ViviDecompiler *dec, ViviDecompilerBlock *block)
{
  g_assert (vivi_decompiler_block_get_n_incoming (block) == 0);
  dec->blocks = g_list_remove (dec->blocks, block);
  g_object_unref (block);
}

/*  ONE
 *   |              ==>   BLOCK
 *  TWO
 */
static gboolean
vivi_decompiler_merge_lines (ViviDecompiler *dec)
{
  ViviDecompilerBlock *block, *next;
  ViviCodeValue *val;
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

    vivi_decompiler_block_set_next (block, vivi_decompiler_block_get_next (next));
    val = vivi_decompiler_block_get_branch_condition (next);
    if (val) {
      vivi_decompiler_block_set_branch (block, 
	  vivi_decompiler_block_get_branch (next),
	  g_object_ref (val));
    }
    vivi_decompiler_block_set_next (next, NULL);
    vivi_decompiler_block_set_branch (next, NULL, NULL);
    vivi_decompiler_block_add_to_block (next, VIVI_CODE_BLOCK (block));
    vivi_decompiler_purge_block (dec, next);
    result = TRUE;
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
  ViviCodeIf *if_stmt;
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
    if_stmt = VIVI_CODE_IF (vivi_code_if_new (g_object_ref (
	  vivi_decompiler_block_get_branch_condition (block))));
    vivi_decompiler_block_set_branch (block, NULL, NULL);
    vivi_decompiler_block_set_next (block, 
	vivi_decompiler_block_get_next (if_block ? if_block : else_block));
    if (if_block) {
      ViviCodeBlock *tmp = vivi_code_block_new ();

      vivi_decompiler_block_set_next (if_block, NULL);
      vivi_decompiler_block_set_branch (if_block, NULL, NULL);
      vivi_decompiler_block_add_to_block (if_block, tmp);
      vivi_decompiler_purge_block (dec, if_block);
      vivi_code_if_set_if (if_stmt, VIVI_CODE_STATEMENT (tmp));
    }
    if (else_block) {
      ViviCodeBlock *tmp = vivi_code_block_new ();

      vivi_decompiler_block_set_next (else_block, NULL);
      vivi_decompiler_block_set_branch (else_block, NULL, NULL);
      vivi_decompiler_block_add_to_block (else_block, tmp);
      vivi_decompiler_purge_block (dec, else_block);
      vivi_code_if_set_else (if_stmt, VIVI_CODE_STATEMENT (tmp));
    }
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block),
	VIVI_CODE_STATEMENT (if_stmt));
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
  g_list_foreach (dec->blocks, (GFunc) g_object_unref, NULL);
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

ViviCodeBlock *
vivi_decompiler_get_block (ViviDecompiler *dec)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER (dec), NULL);

  return dec->blocks->data;
}

