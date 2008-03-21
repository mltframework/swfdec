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
#include "vivi_code_comment.h"
#include "vivi_code_goto.h"
#include "vivi_code_if.h"
#include "vivi_code_label.h"

G_DEFINE_TYPE (ViviDecompilerBlock, vivi_decompiler_block, VIVI_TYPE_CODE_BLOCK)

static void
vivi_decompiler_block_dispose (GObject *object)
{
  ViviDecompilerBlock *block = VIVI_DECOMPILER_BLOCK (object);

  vivi_decompiler_block_reset (block);
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
vivi_decompiler_block_reset (ViviDecompilerBlock *block)
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
  block->endpc = NULL;
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

  if (block->branch) {
    block->branch->incoming--;
    g_object_unref (block->branch_condition);
  }
  block->branch = branch;
  block->branch_condition = branch_condition;
  if (branch)
    branch->incoming++;
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
    const char *format, ...)
{
  ViviCodeToken *token;
  va_list varargs;
  char *s;

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);

  token = vivi_code_comment_new (s);
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
  return pc >= block->startpc && pc < block->endpc;
}

void
vivi_decompiler_block_finish (ViviDecompilerBlock *block, const ViviDecompilerState *state)
{
  g_return_if_fail (block->endpc == NULL);

  block->endpc = vivi_decompiler_state_get_pc (state);
}

gboolean
vivi_decompiler_block_is_finished (ViviDecompilerBlock *block)
{
  return block->endpc != NULL;
}

const ViviDecompilerState *
vivi_decompiler_block_get_start_state (ViviDecompilerBlock *block)
{
  return block->start;
}

guint
vivi_decompiler_block_get_n_incoming (ViviDecompilerBlock *block)
{
  return block->incoming;
}

void
vivi_decompiler_block_add_to_block (ViviDecompilerBlock *block,
    ViviCodeBlock *target)
{
  guint i;

  g_return_if_fail (VIVI_IS_DECOMPILER_BLOCK (block));
  g_return_if_fail (VIVI_IS_CODE_BLOCK (target));

  for (i = 0; i < vivi_code_block_get_n_statements (VIVI_CODE_BLOCK (block)); i++) {
    vivi_code_block_add_statement (target, 
	vivi_code_block_get_statement (VIVI_CODE_BLOCK (block), i));
  }
  if (block->branch) {
    ViviCodeToken *token = vivi_code_if_new (
	g_object_ref (block->branch_condition));
    vivi_decompiler_block_force_label (block->branch);
    vivi_code_if_set_if (VIVI_CODE_IF (token), VIVI_CODE_STATEMENT (
	  vivi_code_goto_new (g_object_ref (
		vivi_decompiler_block_get_label (block->branch)))));
    vivi_code_block_add_statement (target, VIVI_CODE_STATEMENT (token));
  }
  if (block->next) {
    vivi_decompiler_block_force_label (block->next);
    vivi_code_block_add_statement (target, VIVI_CODE_STATEMENT (
	  vivi_code_goto_new (g_object_ref (
	      vivi_decompiler_block_get_label (block->next)))));
  }
}

