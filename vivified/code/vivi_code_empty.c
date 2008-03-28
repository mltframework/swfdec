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

#include "vivi_code_empty.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCodeEmpty, vivi_code_empty, VIVI_TYPE_CODE_STATEMENT)

static ViviCodeStatement *
vivi_code_empty_optimize (ViviCodeStatement *statement)
{
  // FIXME
  return statement;
}

static gboolean
vivi_code_empty_needs_braces (ViviCodeStatement *stmt)
{
  return FALSE;
}

static void
vivi_code_empty_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  vivi_code_printer_print (printer, ";");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_empty_class_init (ViviCodeEmptyClass *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeStatementClass *statement_class = VIVI_CODE_STATEMENT_CLASS (klass);

  token_class->print = vivi_code_empty_print;

  statement_class->optimize = vivi_code_empty_optimize;
  statement_class->needs_braces = vivi_code_empty_needs_braces;
}

static void
vivi_code_empty_init (ViviCodeEmpty *token)
{
}

ViviCodeStatement *
vivi_code_empty_new (void)
{
  ViviCodeEmpty *stmt;

  stmt = g_object_new (VIVI_TYPE_CODE_EMPTY, NULL);

  return VIVI_CODE_STATEMENT (stmt);
}
