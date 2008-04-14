/* Vivified
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_code_builtin_value_statement.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"

G_DEFINE_ABSTRACT_TYPE (ViviCodeBuiltinValueStatement, vivi_code_builtin_value_statement, VIVI_TYPE_CODE_BUILTIN_STATEMENT)

static void
vivi_code_builtin_value_statement_dispose (GObject *object)
{
  ViviCodeBuiltinValueStatement *stmt =
    VIVI_CODE_BUILTIN_VALUE_STATEMENT (object);

  g_assert (stmt->value != NULL);
  g_object_unref (stmt->value);

  G_OBJECT_CLASS (vivi_code_builtin_value_statement_parent_class)->dispose (object);
}

static void
vivi_code_builtin_value_statement_print (ViviCodeToken *token,
    ViviCodePrinter *printer)
{
  ViviCodeBuiltinStatementClass *klass =
    VIVI_CODE_BUILTIN_STATEMENT_CLASS (token);
  ViviCodeBuiltinValueStatement *stmt =
    VIVI_CODE_BUILTIN_VALUE_STATEMENT (token);

  g_assert (klass->function_name != NULL);
  vivi_code_printer_print (printer, klass->function_name);
  vivi_code_printer_print (printer, " (");

  g_assert (stmt->value != NULL);
  vivi_code_printer_print_value (printer, stmt->value, VIVI_PRECEDENCE_COMMA);

  vivi_code_printer_print (printer, ");");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_builtin_value_statement_compile (ViviCodeToken *token,
    ViviCodeCompiler *compiler)
{
  ViviCodeBuiltinStatementClass *klass =
    VIVI_CODE_BUILTIN_STATEMENT_CLASS (token);
  ViviCodeBuiltinValueStatement *stmt =
    VIVI_CODE_BUILTIN_VALUE_STATEMENT (token);

  g_assert (stmt->value != NULL);
  vivi_code_compiler_compile_value (compiler, stmt->value);

  g_assert (klass->bytecode != SWFDEC_AS_ACTION_END);
  vivi_code_compiler_write_empty_action (compiler, klass->bytecode);
}

static void
vivi_code_builtin_value_statement_class_init (
    ViviCodeBuiltinValueStatementClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_builtin_value_statement_dispose;

  token_class->print = vivi_code_builtin_value_statement_print;
  token_class->compile = vivi_code_builtin_value_statement_compile;
}

static void
vivi_code_builtin_value_statement_init (ViviCodeBuiltinValueStatement *stmt)
{
}

void
vivi_code_builtin_value_statement_set_value (
    ViviCodeBuiltinValueStatement *stmt, ViviCodeValue *value)
{
  g_return_if_fail (VIVI_IS_CODE_BUILTIN_VALUE_STATEMENT (stmt));
  g_return_if_fail (VIVI_IS_CODE_VALUE (value));

  if (stmt->value)
    g_object_unref (value);
  stmt->value = g_object_ref (value);
}

ViviCodeValue *
vivi_code_builtin_value_statement_get_value (
    ViviCodeBuiltinValueStatement *stmt)
{
  g_return_val_if_fail (VIVI_IS_CODE_BUILTIN_VALUE_STATEMENT (stmt), NULL);

  return stmt->value;
}
