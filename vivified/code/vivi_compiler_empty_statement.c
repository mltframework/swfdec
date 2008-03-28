/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
 *               2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_compiler_empty_statement.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCompilerEmptyStatement, vivi_compiler_empty_statement, VIVI_TYPE_CODE_STATEMENT)

static ViviCodeStatement *
vivi_compiler_empty_statement_optimize (ViviCodeStatement *statement)
{
  // FIXME
  return statement;
}

static gboolean
vivi_compiler_empty_statement_needs_braces (ViviCodeStatement *stmt)
{
  return FALSE;
}

static void
vivi_compiler_empty_statement_print (ViviCodeToken *token,
    ViviCodePrinter *printer)
{
  vivi_code_printer_print (printer, ";");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_compiler_empty_statement_class_init (
    ViviCompilerEmptyStatementClass *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeStatementClass *statement_class = VIVI_CODE_STATEMENT_CLASS (klass);

  token_class->print = vivi_compiler_empty_statement_print;

  statement_class->optimize = vivi_compiler_empty_statement_optimize;
  statement_class->needs_braces = vivi_compiler_empty_statement_needs_braces;
}

static void
vivi_compiler_empty_statement_init (ViviCompilerEmptyStatement *token)
{
}

ViviCodeStatement *
vivi_compiler_empty_statement_new (void)
{
  ViviCompilerEmptyStatement *stmt;

  stmt = g_object_new (VIVI_TYPE_COMPILER_EMPTY, NULL);

  return VIVI_CODE_STATEMENT (stmt);
}
