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

#include "vivi_code_try.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm.h"
#include "vivi_code_asm_code_default.h"
#include "vivi_code_asm_try.h"

G_DEFINE_TYPE (ViviCodeTry, vivi_code_try, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_try_dispose (GObject *object)
{
  ViviCodeTry *try_ = VIVI_CODE_TRY (object);

  g_object_unref (try_->try_statement);
  g_free (try_->catch_name);
  if (try_->catch_statement)
    g_object_unref (try_->catch_statement);
  if (try_->finally_statement)
    g_object_unref (try_->finally_statement);

  G_OBJECT_CLASS (vivi_code_try_parent_class)->dispose (object);
}

static void
vivi_code_try_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeTry *try_ = VIVI_CODE_TRY (token);

  vivi_code_printer_print (printer, "try {");
  vivi_code_printer_new_line (printer, FALSE);
  vivi_code_printer_print_token (printer,
      VIVI_CODE_TOKEN (try_->try_statement));

  if (try_->catch_statement) {
    vivi_code_printer_print (printer, "} catch (");
    vivi_code_printer_print (printer, try_->catch_name);
    vivi_code_printer_print (printer, ") {");
    vivi_code_printer_new_line (printer, FALSE);
    vivi_code_printer_print_token (printer,
	VIVI_CODE_TOKEN (try_->catch_statement));
  }

  if (try_->finally_statement) {
    vivi_code_printer_print (printer, "} finally {");
    vivi_code_printer_new_line (printer, FALSE);
    vivi_code_printer_print_token (printer,
	VIVI_CODE_TOKEN (try_->finally_statement));
  }

  vivi_code_printer_print (printer, "};");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_try_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeTry *try_ = VIVI_CODE_TRY (token);
  ViviCodeLabel *label_catch, *label_finally, *label_end;

  if (try_->catch_statement) {
    label_catch = vivi_code_compiler_create_label (compiler, "try_catch");
  } else {
    label_catch = NULL;
  }
  if (try_->finally_statement) {
    label_finally = vivi_code_compiler_create_label (compiler, "try_finally");
  } else {
    label_finally = NULL;
  }
  label_end = vivi_code_compiler_create_label (compiler, "try_end");

  vivi_code_compiler_take_code (compiler, vivi_code_asm_try_new (label_catch,
	label_finally, label_end, try_->catch_name));

  vivi_code_compiler_compile_statement (compiler, try_->try_statement);
  if (try_->catch_statement) {
    vivi_code_compiler_take_code (compiler, VIVI_CODE_ASM (label_catch));
    vivi_code_compiler_compile_statement (compiler, try_->catch_statement);
  }
  if (try_->finally_statement) {
    vivi_code_compiler_take_code (compiler, VIVI_CODE_ASM (label_finally));
    vivi_code_compiler_compile_statement (compiler, try_->finally_statement);
  }
  vivi_code_compiler_take_code (compiler, VIVI_CODE_ASM (label_end));
}

static void
vivi_code_try_class_init (ViviCodeTryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_try_dispose;

  token_class->print = vivi_code_try_print;
  token_class->compile = vivi_code_try_compile;
}

static void
vivi_code_try_init (ViviCodeTry *token)
{
}

ViviCodeStatement *
vivi_code_try_new (ViviCodeStatement *try_statement, const char *catch_name,
    ViviCodeStatement *catch_statement, ViviCodeStatement *finally_statement)
{
  ViviCodeTry *try_ = g_object_new (VIVI_TYPE_CODE_TRY, NULL);

  try_->try_statement = g_object_ref (try_statement);
  try_->catch_name = g_strdup (catch_name);
  if (catch_statement)
    try_->catch_statement = g_object_ref (catch_statement);
  if (finally_statement)
    try_->finally_statement = g_object_ref (finally_statement);

  return VIVI_CODE_STATEMENT (try_);
}
