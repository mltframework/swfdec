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

  g_queue_foreach (block->statements, (GFunc) g_object_unref, NULL);
  g_queue_free (block->statements);

  G_OBJECT_CLASS (vivi_code_block_parent_class)->dispose (object);
}

static void
vivi_code_block_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeBlock *block = VIVI_CODE_BLOCK (token);
  guint length;

  length = g_queue_get_length (block->statements);
  if (length > 1) {
    vivi_code_printer_new_line (printer, FALSE);
    vivi_code_printer_print (printer, "{");
  }
  vivi_code_printer_push_indentation (printer);
  if (length == 0) {
    vivi_code_printer_new_line (printer, FALSE);
    vivi_code_printer_print (printer, ";");
  } else {
    GList *walk;

    for (walk = g_queue_peek_head_link (block->statements); walk; walk = walk->next) {
      vivi_code_printer_print_token (printer, walk->data);
    }
  }
  vivi_code_printer_pop_indentation (printer);
  if (length > 1) {
    vivi_code_printer_new_line (printer, FALSE);
    vivi_code_printer_print (printer, "}");
  }
}

static void
vivi_code_block_class_init (ViviCodeBlockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_block_dispose;

  token_class->print = vivi_code_block_print;
}

static void
vivi_code_block_init (ViviCodeBlock *block)
{
  block->statements = g_queue_new ();
}

ViviCodeBlock *
vivi_code_block_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_BLOCK, NULL);
}

void
vivi_code_block_add_statement (ViviCodeBlock *block,
    ViviCodeStatement *statement)
{
  g_return_if_fail (VIVI_IS_CODE_BLOCK (block));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (statement));

  g_queue_push_tail (block->statements, statement);
}

