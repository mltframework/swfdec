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

#include "vivi_code_block.h"
#include "vivi_code_comment.h"
#include "vivi_code_label.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCodeBlock, vivi_code_block, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_block_dispose (GObject *object)
{
  ViviCodeBlock *block = VIVI_CODE_BLOCK (object);
  guint i;

  for (i = 0; i < block->statements->len; i++) {
    g_object_unref (g_ptr_array_index (block->statements, i));
  }
  g_ptr_array_free (block->statements, TRUE);

  G_OBJECT_CLASS (vivi_code_block_parent_class)->dispose (object);
}

static ViviCodeStatement *
vivi_code_block_optimize (ViviCodeStatement *stmt)
{
  ViviCodeBlock *block = VIVI_CODE_BLOCK (stmt);

  if (block->statements->len == 0)
    return NULL;
  if (block->statements->len == 1)
    return g_object_ref (g_ptr_array_index (block->statements, 0));

  return g_object_ref (block);
}

static gboolean
vivi_code_block_needs_braces (ViviCodeStatement *stmt)
{
  ViviCodeBlock *block = VIVI_CODE_BLOCK (stmt);

  if (block->statements->len == 0)
    return FALSE;
  if (block->statements->len > 1)
    return TRUE;

  return vivi_code_statement_needs_braces (g_ptr_array_index (block->statements, 0));
}

static void
vivi_code_block_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeBlock *block = VIVI_CODE_BLOCK (token);

  if (block->statements->len == 0) {
    vivi_code_printer_print (printer, ";");
    vivi_code_printer_new_line (printer, FALSE);
  } else {
    guint i;

    for (i = 0; i < block->statements->len; i++) {
      vivi_code_printer_print_token (printer, g_ptr_array_index (block->statements, i));
    }
  }
}

static void
vivi_code_block_class_init (ViviCodeBlockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeStatementClass *statement_class = VIVI_CODE_STATEMENT_CLASS (klass);

  object_class->dispose = vivi_code_block_dispose;

  token_class->print = vivi_code_block_print;

  statement_class->optimize = vivi_code_block_optimize;
  statement_class->needs_braces = vivi_code_block_needs_braces;
}

static void
vivi_code_block_init (ViviCodeBlock *block)
{
  block->statements = g_ptr_array_new ();
}

ViviCodeBlock *
vivi_code_block_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_BLOCK, NULL);
}

guint
vivi_code_block_get_n_statements (ViviCodeBlock *block)
{
  g_return_val_if_fail (VIVI_IS_CODE_BLOCK (block), 0);

  return block->statements->len;
}

ViviCodeStatement *
vivi_code_block_get_statement (ViviCodeBlock *block, guint i)
{
  g_return_val_if_fail (VIVI_IS_CODE_BLOCK (block), NULL);
  g_return_val_if_fail (i < block->statements->len, NULL);

  return g_ptr_array_index (block->statements, i);
}

void
vivi_code_block_insert_statement (ViviCodeBlock *block, guint i, ViviCodeStatement *statement)
{
  g_return_if_fail (VIVI_IS_CODE_BLOCK (block));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (block));

  g_object_ref (statement);
  if (i >= block->statements->len) {
    g_ptr_array_add (block->statements, statement);
  } else {
    guint after = block->statements->len - i;
    g_ptr_array_set_size (block->statements, block->statements->len + 1);
    memmove (&g_ptr_array_index (block->statements, i + 1),
	&g_ptr_array_index (block->statements, i), sizeof (gpointer) * after);
    g_ptr_array_index (block->statements, i) = statement;
  }
}

void
vivi_code_block_remove_statement (ViviCodeBlock *block, ViviCodeStatement *statement)
{
  g_return_if_fail (VIVI_IS_CODE_BLOCK (block));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (block));

  if (g_ptr_array_remove (block->statements, statement))
    g_object_unref (statement);
  else
    g_return_if_reached ();
}

