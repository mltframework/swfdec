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

#include "vivi_decompiler_block.h"

struct _ViviDecompilerBlock {
  ViviDecompilerState *	start;		/* starting state */
  GPtrArray *		lines;		/* lines parsed from this block */
  guint			incoming;	/* number of incoming blocks */
  char *		label;		/* label generated for this block, so we can goto it */
  const guint8 *	startpc;	/* pointer to first command in block */
  /* set by parsing the block */
  const guint8 *	endpc;		/* pointer to after last parsed command or NULL if not parsed yet */
  ViviDecompilerBlock *	next;		/* block following this one or NULL if returning */
  ViviDecompilerBlock *	branch;		/* NULL or block branched to i if statement */
  ViviDecompilerValue *	branch_condition;/* NULL or value for deciding if a branch should be taken */
};

void
vivi_decompiler_block_reset (ViviDecompilerBlock *block)
{
  guint i;

  for (i = 0; i < block->lines->len; i++) {
    g_free (g_ptr_array_index (block->lines, i));
  }
  g_ptr_array_set_size (block->lines, 0);
  vivi_decompiler_block_set_next (block, NULL);
  vivi_decompiler_block_set_branch (block, NULL, NULL);
  block->endpc = NULL;
}

void
vivi_decompiler_block_free (ViviDecompilerBlock *block)
{
  vivi_decompiler_block_reset (block);
  vivi_decompiler_state_free (block->start);
  g_ptr_array_free (block->lines, TRUE);
  g_free (block->label);
  g_slice_free (ViviDecompilerBlock, block);
}

ViviDecompilerBlock *
vivi_decompiler_block_new (ViviDecompilerState *state)
{
  ViviDecompilerBlock *block;

  block = g_slice_new0 (ViviDecompilerBlock);
  block->start = state;
  block->startpc = vivi_decompiler_state_get_pc (state);
  block->lines = g_ptr_array_new ();

  return block;
}

const char *
vivi_decompiler_block_get_label (const ViviDecompilerBlock *block)
{
  return block->label;
}

void
vivi_decompiler_block_force_label (ViviDecompilerBlock *block)
{
  /* NB: label must be unique for the function we parse */
  if (block->label == NULL)
    block->label = g_strdup_printf ("label_%p", block);
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
    ViviDecompilerValue *branch_condition)
{
  g_return_if_fail ((branch != NULL) ^ (branch_condition == NULL));

  if (block->branch) {
    block->branch->incoming--;
    vivi_decompiler_value_free (block->branch_condition);
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

const ViviDecompilerValue *
vivi_decompiler_block_get_branch_condition (ViviDecompilerBlock *block)
{
  return block->branch_condition;
}

void
vivi_decompiler_block_emit_error (ViviDecompilerBlock *block,
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

void
vivi_decompiler_block_emit (ViviDecompilerBlock *block, ViviDecompilerState *state,
    const char *format, ...)
{
  va_list varargs;
  char *s;

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);

  g_ptr_array_add (block->lines, s);
}

const guint8 *
vivi_decompiler_block_get_start (const ViviDecompilerBlock *block)
{
  return block->startpc;
}

gboolean
vivi_decompiler_block_contains (const ViviDecompilerBlock *block, const guint8 *pc)
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
vivi_decompiler_block_is_finished (const ViviDecompilerBlock *block)
{
  return block->endpc != NULL;
}

const ViviDecompilerState *
vivi_decompiler_block_get_start_state (const ViviDecompilerBlock *block)
{
  return block->start;
}

guint
vivi_decompiler_block_get_n_lines (ViviDecompilerBlock *block)
{
  return block->lines->len;
}

const char *
vivi_decompiler_block_get_line (ViviDecompilerBlock *block, guint i)
{
  return g_ptr_array_index (block->lines, i);
}

guint
vivi_decompiler_block_get_n_incoming (const ViviDecompilerBlock *block)
{
  return block->incoming;
}
