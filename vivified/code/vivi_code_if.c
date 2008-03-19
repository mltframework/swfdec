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

#include "vivi_code_if.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCodeIf, vivi_code_if, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_if_dispose (GObject *object)
{
  ViviCodeIf *stmt = VIVI_CODE_IF (object);

  g_object_unref (stmt->condition);
  if (stmt->if_statement)
    g_object_unref (stmt->if_statement);
  if (stmt->else_statement)
    g_object_unref (stmt->else_statement);

  G_OBJECT_CLASS (vivi_code_if_parent_class)->dispose (object);
}

static void
vivi_code_if_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeIf *stmt = VIVI_CODE_IF (token);

  vivi_code_printer_new_line (printer, FALSE);
  vivi_code_printer_print (printer, "if (");
  vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (stmt->condition));
  vivi_code_printer_print (printer, ")");
  if (stmt->if_statement) {
    vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (stmt->if_statement));
  } else {
    vivi_code_printer_push_indentation (printer);
    vivi_code_printer_new_line (printer, FALSE);
    vivi_code_printer_print (printer, ";");
    vivi_code_printer_pop_indentation (printer);
  }
  if (stmt->else_statement) {
    vivi_code_printer_new_line (printer, FALSE);
    vivi_code_printer_print (printer, "else");
    vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (stmt->else_statement));
  }
}

static void
vivi_code_if_class_init (ViviCodeIfClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_if_dispose;

  token_class->print = vivi_code_if_print;
}

static void
vivi_code_if_init (ViviCodeIf *token)
{
}

ViviCodeToken *
vivi_code_if_new (ViviCodeValue *condition)
{
  ViviCodeIf *stmt;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (condition), NULL);

  stmt = g_object_new (VIVI_TYPE_CODE_IF, NULL);
  stmt->condition = condition;

  return VIVI_CODE_TOKEN (stmt);
}

void
vivi_code_if_set_if (ViviCodeIf *if_stmt, ViviCodeStatement *statement)
{
  g_return_if_fail (VIVI_IS_CODE_IF (if_stmt));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (statement));

  if (if_stmt->if_statement)
    g_object_unref (if_stmt->if_statement);
  if_stmt->if_statement = statement;
}

void
vivi_code_if_set_else (ViviCodeIf *if_stmt, ViviCodeStatement *statement)
{
  g_return_if_fail (VIVI_IS_CODE_IF (if_stmt));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (statement));

  if (if_stmt->else_statement)
    g_object_unref (if_stmt->else_statement);
  if_stmt->else_statement = statement;
}

