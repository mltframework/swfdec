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

#include "vivi_code_function_declaration.h"
#include "vivi_code_constant.h"
#include "vivi_code_label.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_function2.h"

#include "vivi_decompiler.h"

G_DEFINE_TYPE (ViviCodeFunctionDeclaration, vivi_code_function_declaration, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_function_declaration_dispose (GObject *object)
{
  ViviCodeFunctionDeclaration *function =
    VIVI_CODE_FUNCTION_DECLARATION (object);
  guint i;

  g_free (function->name);

  if (function->body)
    g_object_unref (function->body);

  for (i = 0; i < function->arguments->len; i++) {
    g_free (g_ptr_array_index (function->arguments, i));
  }
  g_ptr_array_free (function->arguments, TRUE);

  G_OBJECT_CLASS (vivi_code_function_declaration_parent_class)->dispose (object);
}

static void
vivi_code_function_declaration_print (ViviCodeToken *token,
    ViviCodePrinter *printer)
{
  ViviCodeFunctionDeclaration *function =
    VIVI_CODE_FUNCTION_DECLARATION (token);
  guint i;

  vivi_code_printer_print (printer, "function ");
  vivi_code_printer_print (printer, function->name);

  vivi_code_printer_print (printer, " (");
  for (i = 0; i < function->arguments->len; i++) {
    if (i != 0)
      vivi_code_printer_print (printer, ", ");
    vivi_code_printer_print (printer,
	g_ptr_array_index (function->arguments, i));
  }
  vivi_code_printer_print (printer, ") {");
  vivi_code_printer_new_line (printer, FALSE);

  if (function->body) {
    vivi_code_printer_push_indentation (printer);
    vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (function->body));
    vivi_code_printer_pop_indentation (printer);
  }

  vivi_code_printer_print (printer, "}");
}

static void
vivi_code_function_declaration_compile (ViviCodeToken *token,
    ViviCodeCompiler *compiler)
{
  ViviCodeFunctionDeclaration *function =
    VIVI_CODE_FUNCTION_DECLARATION (token);
  ViviCodeLabel *label_end;
  ViviCodeAsm *code;
  guint i;

  label_end = vivi_code_compiler_create_label (compiler, "function_end");

  code = vivi_code_asm_function2_new (label_end, function->name, 0, 0);
  for (i = 0; i < function->arguments->len; i++) {
    vivi_code_asm_function2_add_argument (VIVI_CODE_ASM_FUNCTION2 (code),
	g_ptr_array_index (function->arguments, i), 0);
  }

  vivi_code_compiler_take_code (compiler, code);

  if (function->body != NULL)
    vivi_code_compiler_compile_statement (compiler, function->body);

  vivi_code_compiler_take_code (compiler, VIVI_CODE_ASM (label_end));
}

static void
vivi_code_function_declaration_class_init (
    ViviCodeFunctionDeclarationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_function_declaration_dispose;

  token_class->print = vivi_code_function_declaration_print;
  token_class->compile = vivi_code_function_declaration_compile;
}

static void
vivi_code_function_declaration_init (ViviCodeFunctionDeclaration *function)
{
  function->arguments = g_ptr_array_new ();
}

ViviCodeStatement *
vivi_code_function_declaration_new (const char *name)
{
  ViviCodeStatement *function;

  g_return_val_if_fail (name != NULL, NULL);

  function = VIVI_CODE_STATEMENT (g_object_new (
	VIVI_TYPE_CODE_FUNCTION_DECLARATION, NULL));
  VIVI_CODE_FUNCTION_DECLARATION (function)->name = g_strdup (name);

  return function;
}

void
vivi_code_function_declaration_set_body (ViviCodeFunctionDeclaration *function,
    ViviCodeStatement *body)
{
  g_return_if_fail (VIVI_IS_CODE_FUNCTION_DECLARATION (function));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (body));

  function->body = g_object_ref (body);
}

void
vivi_code_function_declaration_add_argument (
    ViviCodeFunctionDeclaration *function, const char *name)
{
  g_return_if_fail (VIVI_IS_CODE_FUNCTION_DECLARATION (function));
  g_return_if_fail (name != NULL);

  g_ptr_array_add (function->arguments, g_strdup (name));
}
