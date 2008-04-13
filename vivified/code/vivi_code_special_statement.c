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

#include <string.h>

#include "vivi_code_special_statement.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"

G_DEFINE_ABSTRACT_TYPE (ViviCodeSpecialStatement, vivi_code_special_statement, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_special_statement_print (ViviCodeToken *token,
    ViviCodePrinter *printer)
{
  ViviCodeSpecialStatement *stmt = VIVI_CODE_SPECIAL_STATEMENT (token);
  ViviCodeSpecialStatementClass *klass =
    VIVI_CODE_SPECIAL_STATEMENT_GET_CLASS (stmt);

  g_assert (stmt->name != NULL);
  vivi_code_printer_print (printer, stmt->name);

  vivi_code_printer_print (printer, " (");

  if (klass->get_value != NULL) {
    vivi_code_printer_print_value (printer, klass->get_value (stmt),
	VIVI_PRECEDENCE_COMMA);
  }

  vivi_code_printer_print (printer, ");");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_special_statement_compile (ViviCodeToken *token,
    ViviCodeCompiler *compiler)
{
  ViviCodeSpecialStatement *stmt = VIVI_CODE_SPECIAL_STATEMENT (token);
  ViviCodeSpecialStatementClass *klass =
    VIVI_CODE_SPECIAL_STATEMENT_GET_CLASS (stmt);

  if (klass->get_value != NULL)
    vivi_code_compiler_compile_value (compiler, klass->get_value (stmt));

  g_assert (stmt->action != SWFDEC_AS_ACTION_END);
  vivi_code_compiler_write_empty_action (compiler, stmt->action);
}

static void
vivi_code_special_statement_class_init (ViviCodeSpecialStatementClass *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  token_class->print = vivi_code_special_statement_print;
  token_class->compile = vivi_code_special_statement_compile;
}

static void
vivi_code_special_statement_init (ViviCodeSpecialStatement *stmt)
{
}
