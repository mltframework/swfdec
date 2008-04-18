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
#include <swfdec/swfdec_script_internal.h>

#include "vivi_decompiler_block.h"
#include "vivi_code_assignment.h"
#include "vivi_code_block.h"
#include "vivi_code_comment.h"
#include "vivi_code_goto.h"
#include "vivi_code_if.h"
#include "vivi_code_label.h"
#include "vivi_code_string.h"
#include "vivi_decompiler_unknown.h"

G_DEFINE_TYPE (ViviDecompilerBlock, vivi_decompiler_block, VIVI_TYPE_CODE_BLOCK)

static void
vivi_decompiler_block_dispose (GObject *object)
{
  ViviDecompilerBlock *block = VIVI_DECOMPILER_BLOCK (object);

  g_assert (block->incoming == 0);
  vivi_decompiler_block_reset (block, FALSE);
  vivi_decompiler_state_free (block->start);

  G_OBJECT_CLASS (vivi_decompiler_block_parent_class)->dispose (object);
}

static void
vivi_decompiler_block_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  g_return_if_reached ();
}

static void
vivi_decompiler_block_class_init (ViviDecompilerBlockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_decompiler_block_dispose;

  token_class->print = vivi_decompiler_block_print;
}

static void
vivi_decompiler_block_init (ViviDecompilerBlock *block)
{
}

void
vivi_decompiler_block_reset (ViviDecompilerBlock *block, gboolean generalize_start_state)
{
  ViviCodeBlock *code_block = VIVI_CODE_BLOCK (block);
  guint i, len;
  
  len = vivi_code_block_get_n_statements (code_block);
  for (i = len - 1; i < len; i--) {
    vivi_code_block_remove_statement (code_block,
	vivi_code_block_get_statement (code_block, i));
  }
  vivi_decompiler_block_set_next (block, NULL);
  vivi_decompiler_block_set_branch (block, NULL, NULL);
  if (block->end) {
    vivi_decompiler_state_free (block->end);
    block->end = NULL;
  }
  if (generalize_start_state) {
    len = vivi_decompiler_state_get_stack_depth (block->start);
    for (i = 0; i < len; i++) {
      g_object_unref (vivi_decompiler_state_pop (block->start));
    }
    for (i = 0; i < len; i++) {
      char *s = g_strdup_printf ("$stack%u", len - i);
      vivi_decompiler_state_push (block->start,
	  vivi_decompiler_unknown_new (block, s));
      g_free (s);
    }
  }
}

ViviDecompilerBlock *
vivi_decompiler_block_new (ViviDecompilerState *state)
{
  ViviDecompilerBlock *block;

  g_return_val_if_fail (state != NULL, NULL);

  block = g_object_new (VIVI_TYPE_DECOMPILER_BLOCK, NULL);
  block->start = state;
  block->startpc = vivi_decompiler_state_get_pc (state);

  return block;
}

ViviCodeStatement *
vivi_decompiler_block_get_label (ViviDecompilerBlock *block)
{
  ViviCodeStatement *stmt;

  g_return_val_if_fail (VIVI_IS_DECOMPILER_BLOCK (block), NULL);

  if (vivi_code_block_get_n_statements (VIVI_CODE_BLOCK (block)) == 0)
    return NULL;
  stmt = vivi_code_block_get_statement (VIVI_CODE_BLOCK (block), 0);
  if (!VIVI_IS_CODE_LABEL (stmt))
    return NULL;

  return stmt;
}

void
vivi_decompiler_block_force_label (ViviDecompilerBlock *block)
{
  ViviCodeStatement *stmt;
  char *s;

  g_return_if_fail (VIVI_IS_DECOMPILER_BLOCK (block));

  if (vivi_decompiler_block_get_label (block))
    return;

  s = g_strdup_printf ("label_%p", block);
  stmt = vivi_code_label_new (s);
  g_free (s);
  vivi_code_block_insert_statement (VIVI_CODE_BLOCK (block), 0, stmt);
  g_object_unref (stmt);
}

void
vivi_decompiler_block_set_next (ViviDecompilerBlock *block, ViviDecompilerBlock *next)
{
  if (block->next)
    block->next->incoming--;
  block->next = next;
  if (next)
    next->incoming++;
}

ViviDecompilerBlock *
vivi_decompiler_block_get_next (ViviDecompilerBlock *block)
{
  return block->next;
}

void
vivi_decompiler_block_set_branch (ViviDecompilerBlock *block, ViviDecompilerBlock *branch,
    ViviCodeValue *branch_condition)
{
  g_return_if_fail ((branch != NULL) ^ (branch_condition == NULL));

  if (branch) {
    g_object_ref (branch_condition);
    branch->incoming++;
  }
  if (block->branch) {
    block->branch->incoming--;
    g_object_unref (block->branch_condition);
  }
  block->branch = branch;
  block->branch_condition = branch_condition;
}

ViviDecompilerBlock *
vivi_decompiler_block_get_branch (ViviDecompilerBlock *block)
{
  return block->branch;
}

ViviCodeValue *
vivi_decompiler_block_get_branch_condition (ViviDecompilerBlock *block)
{
  return block->branch_condition;
}

void
vivi_decompiler_block_add_error (ViviDecompilerBlock *block,
    ViviDecompilerState *state, const char *format, ...)
{
  ViviCodeToken *token;
  va_list varargs;
  char *s;

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);

  token = vivi_code_comment_new (s);
  g_printerr ("ERROR: %s\n", s);
  g_free (s);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (token));
  vivi_decompiler_block_finish (block, state);
}

void
vivi_decompiler_block_add_warning (ViviDecompilerBlock *block,
    const char *format, ...)
{
  ViviCodeToken *token;
  va_list varargs;
  char *s;

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);

  token = vivi_code_comment_new (s);
  g_printerr ("WARNING: %s\n", s);
  g_free (s);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (token));
}

const guint8 *
vivi_decompiler_block_get_start (ViviDecompilerBlock *block)
{
  return block->startpc;
}

gboolean
vivi_decompiler_block_contains (ViviDecompilerBlock *block, const guint8 *pc)
{
  const guint8 *check;
  guint code;

  g_return_val_if_fail (VIVI_IS_DECOMPILER_BLOCK (block), FALSE);
  g_return_val_if_fail (pc != NULL, FALSE);

  if (pc == block->startpc)
    return TRUE;
  if (block->end == NULL)
    return FALSE;

  if (pc >= vivi_decompiler_state_get_pc (block->end))
    return FALSE;
  for (check = block->startpc; check < pc; ) {
    code = *check;
    if (code & 0x80) {
      check += (check[1] | check[2] << 8) + 3;
    } else {
      check++;
    }
  }
  return check == pc;
}

void
vivi_decompiler_block_finish (ViviDecompilerBlock *block, const ViviDecompilerState *state)
{
  ViviDecompilerState *new;

  g_return_if_fail (VIVI_IS_DECOMPILER_BLOCK (block));
  g_return_if_fail (state != NULL);

  new = vivi_decompiler_state_copy (state);
  if (block->end)
    vivi_decompiler_state_free (block->end);
  block->end = new;
}

gboolean
vivi_decompiler_block_is_finished (ViviDecompilerBlock *block)
{
  return block->end != NULL;
}

const ViviDecompilerState *
vivi_decompiler_block_get_start_state (ViviDecompilerBlock *block)
{
  return block->start;
}

const ViviDecompilerState *
vivi_decompiler_block_get_end_state (ViviDecompilerBlock *block)
{
  return block->end;
}

guint
vivi_decompiler_block_get_n_incoming (ViviDecompilerBlock *block)
{
  return block->incoming;
}

void
vivi_decompiler_block_add_state_transition (ViviDecompilerBlock *from,
    ViviDecompilerBlock *to, ViviCodeBlock *block)
{
  guint i, len;
  
  len = vivi_decompiler_state_get_stack_depth (to->start);
  if (from->end == NULL) {
    g_printerr ("broken source - no state transition possible\n");
    return;
  }

  for (i = 0; i < len; i++) {
    ViviCodeValue *name;
    ViviCodeValue *val_from;
    ViviDecompilerUnknown *val_to = (ViviDecompilerUnknown *) vivi_decompiler_state_peek_nth (to->start, i);

    if (!VIVI_IS_DECOMPILER_UNKNOWN (val_to) ||
	vivi_decompiler_unknown_get_value (val_to))
      continue;

    val_from = vivi_decompiler_state_peek_nth (from->end, i);
    if (val_from == NULL) {
      g_printerr ("incompatible states - aborting transition\n");
      return;
    }
    if (val_from != VIVI_CODE_VALUE (val_to)) {
      name = vivi_code_string_new (vivi_decompiler_unknown_get_name (val_to));
      vivi_code_block_add_statement (block,
	  vivi_code_assignment_new (NULL, name, val_from));
    }
  }
}

void
vivi_decompiler_block_add_to_block (ViviDecompilerBlock *block,
    ViviCodeBlock *target)
{
  ViviCodeStatement *stmt, *stmt2;
  guint i;

  g_return_if_fail (VIVI_IS_DECOMPILER_BLOCK (block));
  g_return_if_fail (VIVI_IS_CODE_BLOCK (target));

  for (i = 0; i < vivi_code_block_get_n_statements (VIVI_CODE_BLOCK (block)); i++) {
    vivi_code_block_add_statement (target, g_object_ref (
	vivi_code_block_get_statement (VIVI_CODE_BLOCK (block), i)));
  }
  if (block->branch) {
    ViviCodeStatement *b;
    stmt = vivi_code_if_new (block->branch_condition);
    vivi_decompiler_block_force_label (block->branch);

    b = vivi_code_block_new ();
    vivi_decompiler_block_add_state_transition (block, block->branch, VIVI_CODE_BLOCK (b));
    stmt2 = vivi_code_goto_new (VIVI_CODE_LABEL (vivi_decompiler_block_get_label (block->branch)));
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (b), stmt2);
    g_object_unref (stmt2);
    stmt2 = vivi_code_statement_optimize (b);
    g_assert (stmt2);
    g_object_unref (b);
    
    vivi_code_if_set_if (VIVI_CODE_IF (stmt), stmt2);
    g_object_unref (stmt2);
    vivi_code_block_add_statement (target, stmt);
    g_object_unref (stmt);
  }
  if (block->next) {
    vivi_decompiler_block_force_label (block->next);
    vivi_decompiler_block_add_state_transition (block, block->next, target);
    stmt = vivi_code_goto_new (VIVI_CODE_LABEL (vivi_decompiler_block_get_label (block->next)));
    vivi_code_block_add_statement (target, stmt);
    g_object_unref (stmt);
  }
}

gboolean
vivi_decompiler_block_is_compatible (ViviDecompilerBlock *block, const ViviDecompilerState *state)
{
  guint i, n;

  g_return_val_if_fail (VIVI_IS_DECOMPILER_BLOCK (block), FALSE);
  g_return_val_if_fail (state != NULL, FALSE);

  n = vivi_decompiler_state_get_stack_depth (state);
  for (i = 0; i < n; i++) {
    ViviCodeValue *val = vivi_decompiler_state_peek_nth (block->start, i);

    if (VIVI_IS_DECOMPILER_UNKNOWN (val) &&
	vivi_decompiler_unknown_get_block (VIVI_DECOMPILER_UNKNOWN (val)) == block)
      continue;
    if (!vivi_code_value_is_equal (vivi_decompiler_state_peek_nth (state, i), val))
      return FALSE;
  }
  return TRUE;
}

