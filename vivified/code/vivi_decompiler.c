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
#include "vivi_code_block.h"
#include "vivi_code_break.h"
#include "vivi_code_constant.h"
#include "vivi_code_continue.h"
#include "vivi_code_get_url.h"
#include "vivi_code_goto.h"
#include "vivi_code_if.h"
#include "vivi_code_loop.h"
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
	    return TRUE;
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
	    return TRUE;
	  }
	  if (i >= swfdec_constant_pool_size (pool)) {
	    vivi_decompiler_block_add_error (block, "constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (pool));
	    return TRUE;
	  }
	  value = escape_string (swfdec_constant_pool_get (pool, i));
	  break;
	}
      default:
	vivi_decompiler_block_add_error (block, "Push: type %u not implemented", type);
	return TRUE;
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
  val = vivi_code_unary_new (val, '!');
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

	vivi_decompiler_state_add_pc (state, 5);
	if (len != 2) {
	  vivi_decompiler_block_add_error (block, "If action length invalid (is %u, should be 2)", len);
	  vivi_decompiler_block_finish (block, state);
	  return TRUE;
	}
	offset = data[0] | (data[1] << 8);
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

	vivi_decompiler_state_add_pc (state, 5);
	if (len != 2) {
	  vivi_decompiler_block_add_error (block, "Jump action length invalid (is %u, should be 2)", len);
	  vivi_decompiler_block_finish (block, state);
	  return FALSE;
	}
	offset = data[0] | (data[1] << 8);
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
	result = TRUE;
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
vivi_decompiler_find_start_block (GList *list, const guint8 *startpc)
{
  GList *walk;
  
  for (walk = list; walk; walk = walk->next) {
    ViviDecompilerBlock *block = walk->data;

    if (vivi_decompiler_block_get_start (block) == startpc)
      return block;
  }
  g_assert_not_reached ();
  return NULL;
}

static ViviCodeStatement *
vivi_decompiler_merge_blocks_last_resort (GList *list, const guint8 *startpc)
{
  ViviCodeBlock *block;
  ViviDecompilerBlock *current, *next;
  GList *ordered, *walk;

  current = vivi_decompiler_find_start_block (list, startpc);

  ordered = NULL;
  while (current) {
    g_assert (g_list_find (list, current));
    list = g_list_remove (list, current);
    ordered = g_list_prepend (ordered, current);
    next = vivi_decompiler_block_get_branch (current);
    if (next)
      vivi_decompiler_block_force_label (next);
    next = vivi_decompiler_block_get_next (current);
    if (next == NULL || !g_list_find (list, next)) {
      if (next)
	vivi_decompiler_block_force_label (next);
      next = list ? list->data : NULL;
    }
    current = next;
  }
  g_assert (list == NULL);
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
  return VIVI_CODE_STATEMENT (block);
}

static GList *
vivi_decompiler_purge_block (GList *list, ViviDecompilerBlock *block)
{
  g_assert (vivi_decompiler_block_get_n_incoming (block) == 0);
  list = g_list_remove (list, block);
  g_object_unref (block);
  return list;
}

/*  ONE
 *   |              ==>   BLOCK
 *  TWO
 */
static gboolean
vivi_decompiler_merge_lines (ViviDecompiler *dec, GList **list)
{
  ViviDecompilerBlock *block, *next;
  ViviCodeValue *val;
  gboolean result;
  GList *walk;

  result = FALSE;
  for (walk = *list; walk; walk = walk->next) {
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
    *list = vivi_decompiler_purge_block (*list, next);
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
vivi_decompiler_merge_if (ViviDecompiler *dec, GList **list)
{
  ViviDecompilerBlock *block, *if_block, *else_block;
  ViviCodeIf *if_stmt;
  ViviCodeStatement *stmt;
  gboolean result;
  GList *walk;

  result = FALSE;
  for (walk = *list; walk; walk = walk->next) {
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
      *list = vivi_decompiler_purge_block (*list, if_block);
      vivi_code_if_set_if (if_stmt, VIVI_CODE_STATEMENT (tmp));
    }
    if (else_block) {
      ViviCodeBlock *tmp = vivi_code_block_new ();

      vivi_decompiler_block_set_next (else_block, NULL);
      vivi_decompiler_block_set_branch (else_block, NULL, NULL);
      vivi_decompiler_block_add_to_block (else_block, tmp);
      *list = vivi_decompiler_purge_block (*list, else_block);
      vivi_code_if_set_else (if_stmt, VIVI_CODE_STATEMENT (tmp));
    }
    stmt = vivi_code_statement_optimize (VIVI_CODE_STATEMENT (if_stmt));
    g_object_unref (if_stmt);
    if (stmt)
      vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), stmt);
    result = TRUE;
  }

  return result;
}

static ViviCodeStatement *vivi_decompiler_merge_blocks (ViviDecompiler *dec, 
    const guint8 *startpc, GList *blocks);

static gboolean
vivi_decompiler_merge_loops (ViviDecompiler *dec, GList **list)
{
  ViviDecompilerBlock *end, *start, *block, *next;
  gboolean result;
  GList *walk, *walk2;
  const guint8 *loop_start, *loop_end;
  GList *contained, *to_check;
  ViviCodeToken *loop;

  result = FALSE;
  for (walk = dec->blocks; walk; walk = walk->next) {
    end = walk->data;
    /* noone has a branch at the end of a loop */
    if (vivi_decompiler_block_get_branch (end))
      continue;
    start = vivi_decompiler_block_get_next (end);
    /* block that just returns - no loop at all */
    if (start == NULL)
      continue;
    /* not a jump backwards, so no loop end */
    if (vivi_decompiler_block_get_start (start) >
	vivi_decompiler_block_get_start (end))
      continue;
    /* We've found that "end" is a jump backwards. Now, this can be 3 things:
     * a) the end of a loop
     *    Woohoo, we've found our loop end.
     * b) a "continue" statement
     *    Wait, we need to find where the lop ends!
     * c) a goto backwards
     *    Whoops, we need to cleanly exit this loop!
     * In case a) and b), "start" will already be correct. So we'll assume that
     * it is and go from there.
     */
    loop_start = vivi_decompiler_block_get_start (start);
    /* this is just a rough guess for now */
    loop_end = vivi_decompiler_block_get_start (end);
    /* let's find the rest of the loop */
    to_check = g_list_prepend (contained, start);
    contained = NULL;
    while (to_check) {
      block = to_check->data;
      to_check = g_list_remove (to_check, block);
      /* jump to before start?! */
      if (vivi_decompiler_block_get_start (block) < loop_start) {
	g_list_free (contained);
	g_list_free (to_check);
	goto failed;
      }
      /* found a new end of the loop */
      if (vivi_decompiler_block_get_start (block) > loop_end) {
	ViviDecompilerBlock *swap;
	loop_end = vivi_decompiler_block_get_start (block);
	swap = block;
	block = end;
	end = swap;
      }
      contained = g_list_prepend (contained, block);
      next = vivi_decompiler_block_get_next (block);
      if (next && next != end && 
	  !g_list_find (contained, next) && 
          !g_list_find (to_check, next))
	to_check = g_list_prepend (to_check, next);
      next = vivi_decompiler_block_get_branch (block);
      if (next && next != end && 
	  !g_list_find (contained, next) && 
          !g_list_find (to_check, next))
	to_check = g_list_prepend (to_check, next);
    }
    contained = g_list_reverse (contained);
    contained = g_list_remove (contained, start);
    /* now we have:
     * contained: contains all the blocks contained in the loop
     * start: starting block
     * end: end block - where "breaks" go to
     */
    loop = vivi_code_loop_new ();
    /* check if the first block is just a branch, in that case it's the
     * loop condition */
    if (vivi_code_block_get_n_statements (VIVI_CODE_BLOCK (start)) == 0 &&
	(vivi_decompiler_block_get_branch (start) == end ||
	 (vivi_decompiler_block_get_branch (start) &&
	  vivi_decompiler_block_get_next (start) == end))) {
      if (vivi_decompiler_block_get_branch (start) == end) {
	ViviCodeValue *value = vivi_code_unary_new (g_object_ref (
	    vivi_decompiler_block_get_branch_condition (start)), '!');
	vivi_code_loop_set_condition (VIVI_CODE_LOOP (loop),
	    vivi_code_value_optimize (value, SWFDEC_AS_TYPE_BOOLEAN));
	g_object_unref (value);
      } else {
	vivi_code_loop_set_condition (VIVI_CODE_LOOP (loop), g_object_ref (
	    vivi_decompiler_block_get_branch_condition (start)));
	vivi_decompiler_block_set_next (start,
	    vivi_decompiler_block_get_branch (start));
      }
      vivi_decompiler_block_set_branch (start, NULL, NULL);
      loop_start = vivi_decompiler_block_get_start (
	  vivi_decompiler_block_get_next (start));
      vivi_code_block_add_statement (VIVI_CODE_BLOCK (start), VIVI_CODE_STATEMENT (loop));
      vivi_decompiler_block_set_next (start, end);
    } else {
      /* FIXME: for (;;) loop */
      contained = g_list_prepend (contained, start);
      block = vivi_decompiler_block_new (vivi_decompiler_state_copy (
	    vivi_decompiler_block_get_start_state (start)));
      vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (loop));
      for (walk2 = *list; walk2; walk2 = walk2->next) {
	if (walk2->data == start) {
	  walk2->data = block;
	  continue;
	}
	if (vivi_decompiler_block_get_branch (walk2->data) == start)
	  vivi_decompiler_block_set_branch (walk2->data, block,
	      g_object_ref (vivi_decompiler_block_get_branch_condition (walk2->data)));
	if (vivi_decompiler_block_get_next (walk2->data) == start)
	  vivi_decompiler_block_set_next (walk2->data, block);
      }
      vivi_decompiler_block_set_next (block, end);
    }

    /* break all connections to the outside */
    for (walk2 = contained; walk2; walk2 = walk2->next) {
      block = walk2->data;
      *list = g_list_remove (*list, block);
      next = vivi_decompiler_block_get_branch (block);
      if (next && !g_list_find (contained, next)) {
	ViviCodeStatement *stmt = VIVI_CODE_STATEMENT (vivi_code_if_new (
	    g_object_ref (vivi_decompiler_block_get_branch_condition (block))));
	if (next == start) {
	  vivi_code_if_set_if (VIVI_CODE_IF (stmt), vivi_code_continue_new ());
	} else if (next == end) {
	  vivi_code_if_set_if (VIVI_CODE_IF (stmt), vivi_code_break_new ());
	} else {
	  vivi_decompiler_block_force_label (next);
	  vivi_code_if_set_if (VIVI_CODE_IF (stmt), VIVI_CODE_STATEMENT (vivi_code_goto_new (
		VIVI_CODE_LABEL (vivi_decompiler_block_get_label (next)))));
	}
	vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), stmt);
	vivi_decompiler_block_set_branch (block, NULL, NULL);
      }
      next = vivi_decompiler_block_get_next (block);
      if (next && !g_list_find (contained, next)) {
	if (next == start) {
	  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), vivi_code_continue_new ());
	} else if (next == end) {
	  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), vivi_code_break_new ());
	} else {
	  vivi_decompiler_block_force_label (next);
	  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (
		vivi_code_goto_new (VIVI_CODE_LABEL (vivi_decompiler_block_get_label (next)))));
	}
	vivi_decompiler_block_set_next (block, NULL);
      }
    }
    /* break all connections from the outside */
    for (walk2 = *list; walk2; walk2 = walk2->next) {
      block = walk2->data;
      next = vivi_decompiler_block_get_branch (block);
      if (next && !g_list_find (*list, next)) {
	ViviCodeStatement *stmt = VIVI_CODE_STATEMENT (vivi_code_if_new (
	    g_object_ref (vivi_decompiler_block_get_branch_condition (block))));
	vivi_decompiler_block_force_label (next);
	vivi_code_if_set_if (VIVI_CODE_IF (stmt), VIVI_CODE_STATEMENT (vivi_code_goto_new (
	      VIVI_CODE_LABEL (vivi_decompiler_block_get_label (next)))));
	vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), stmt);
	vivi_decompiler_block_set_branch (block, NULL, NULL);
      }
      next = vivi_decompiler_block_get_next (block);
      if (next && !g_list_find (*list, next)) {
	vivi_decompiler_block_force_label (next);
	vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (
	      vivi_code_goto_new (VIVI_CODE_LABEL (vivi_decompiler_block_get_label (next)))));
	vivi_decompiler_block_set_next (block, NULL);
      }
    }
    vivi_code_loop_set_statement (VIVI_CODE_LOOP (loop), vivi_decompiler_merge_blocks (dec, loop_start,
	  contained));

    return TRUE;
failed:
    continue;
  }
  return FALSE;
}

static ViviCodeStatement *
vivi_decompiler_merge_blocks (ViviDecompiler *dec, const guint8 *startpc, GList *blocks)
{
  gboolean restart;

  DUMP_BLOCKS (dec);

  do {
    restart = FALSE;

    restart |= vivi_decompiler_merge_lines (dec, &blocks);
    restart |= vivi_decompiler_merge_if (dec, &blocks);
    restart |= vivi_decompiler_merge_loops (dec, &blocks);
  } while (restart);

  DUMP_BLOCKS (dec);
  return vivi_decompiler_merge_blocks_last_resort (blocks, startpc);
}

static void
vivi_decompiler_dump_graphviz (ViviDecompiler *dec)
{
  GString *string;
  GList *walk;
  const char *filename;

  filename = g_getenv ("VIVI_DECOMPILER_DUMP");
  if (filename == NULL)
    return;

  string = g_string_new ("digraph G\n{\n");
  g_string_append (string, "  node [ shape = box ]\n");
  for (walk = dec->blocks; walk; walk = walk->next) {
    g_string_append_printf (string, "  node%p\n", walk->data);
  }
  g_string_append (string, "\n");
  for (walk = dec->blocks; walk; walk = walk->next) {
    ViviDecompilerBlock *block, *next;

    block = walk->data;
    next = vivi_decompiler_block_get_next (block);
    if (next)
      g_string_append_printf (string, "  node%p -> node%p\n", block, next);
    next = vivi_decompiler_block_get_branch (block);
    if (next)
      g_string_append_printf (string, "  node%p -> node%p\n", block, next);
  }
  g_string_append (string, "}\n");
  g_file_set_contents (filename, string->str, string->len, NULL);
  g_string_free (string, TRUE);
}

static void
vivi_decompiler_run (ViviDecompiler *dec)
{
  ViviDecompilerBlock *block;
  ViviDecompilerState *state;
  ViviCodeStatement *stmt;
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

  vivi_decompiler_dump_graphviz (dec);
  stmt = vivi_decompiler_merge_blocks (dec, dec->script->main, dec->blocks);
  dec->blocks = g_list_prepend (NULL, stmt);
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

