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

#include "vivi_code_variable.h"
#include "vivi_code_get.h"
#include "vivi_code_printer.h"
#include "vivi_code_string.h"
#include "vivi_code_undefined.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeVariable, vivi_code_variable, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_variable_dispose (GObject *object)
{
  ViviCodeVariable *variable = VIVI_CODE_VARIABLE (object);

  g_object_unref (variable->name);
  if (variable->value)
    g_object_unref (variable->value);

  G_OBJECT_CLASS (vivi_code_variable_parent_class)->dispose (object);
}

static void
vivi_code_variable_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeVariable *variable = VIVI_CODE_VARIABLE (token);
  char *varname;

  if (VIVI_IS_CODE_CONSTANT (variable->name)) {
    varname = vivi_code_constant_get_variable_name (
	VIVI_CODE_CONSTANT (variable->name));
  } else {
    varname = NULL;
  }

  vivi_code_printer_print (printer, "var ");

  if (varname) {
    vivi_code_printer_print (printer, varname);
    g_free (varname);
  } else {
    // FIXME
    vivi_code_printer_print (printer, "set (");
    vivi_code_printer_print_value (printer, variable->name,
	VIVI_PRECEDENCE_MIN);
    vivi_code_printer_print (printer, ", ");
    vivi_code_printer_print_value (printer, variable->value,
	VIVI_PRECEDENCE_ASSIGNMENT);
    vivi_code_printer_print (printer, ")");
    goto finalize;
  }

  if (variable->value != NULL) {
    vivi_code_printer_print (printer, " = ");
    vivi_code_printer_print_value (printer, variable->value,
	VIVI_PRECEDENCE_ASSIGNMENT);
  }

finalize:
  vivi_code_printer_print (printer, ";");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_variable_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeVariable *variable = VIVI_CODE_VARIABLE (token);
  ViviCodeAsm *code;

  vivi_code_compiler_compile_value (compiler, variable->name);

  if (variable->value != NULL) {
    vivi_code_compiler_compile_value (compiler, variable->value);
    code = vivi_code_asm_define_local_new ();
  } else {
    code = vivi_code_asm_define_local2_new ();
  }
  vivi_code_compiler_take_code (compiler, code);
}

static void
vivi_code_variable_class_init (ViviCodeVariableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_variable_dispose;

  token_class->print = vivi_code_variable_print;
  token_class->compile = vivi_code_variable_compile;
}

static void
vivi_code_variable_init (ViviCodeVariable *variable)
{
}

ViviCodeStatement *
vivi_code_variable_new (ViviCodeValue *name, ViviCodeValue *value)
{
  ViviCodeVariable *variable;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (name), NULL);
  g_return_val_if_fail (value == NULL || VIVI_IS_CODE_VALUE (value), NULL);

  variable = g_object_new (VIVI_TYPE_CODE_VARIABLE, NULL);
  variable->name = g_object_ref (name);
  if (value != NULL)
    variable->value = g_object_ref (value);

  return VIVI_CODE_STATEMENT (variable);
}

ViviCodeStatement *
vivi_code_variable_new_name (const char *name, ViviCodeValue *value)
{
  ViviCodeValue *constant;
  ViviCodeStatement *result;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (value == NULL || VIVI_IS_CODE_VALUE (value), NULL);

  constant = vivi_code_string_new (name);
  result = vivi_code_variable_new (constant, value);
  g_object_unref (constant);

  return result;
}
