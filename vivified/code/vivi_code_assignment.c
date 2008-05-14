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

#include "vivi_code_assignment.h"
#include "vivi_code_get.h"
#include "vivi_code_printer.h"
#include "vivi_code_string.h"
#include "vivi_code_undefined.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeAssignment, vivi_code_assignment, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_assignment_dispose (GObject *object)
{
  ViviCodeAssignment *assignment = VIVI_CODE_ASSIGNMENT (object);

  if (assignment->from)
    g_object_unref (assignment->from);
  g_object_unref (assignment->name);
  g_object_unref (assignment->value);

  G_OBJECT_CLASS (vivi_code_assignment_parent_class)->dispose (object);
}

static void
vivi_code_assignment_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAssignment *assignment = VIVI_CODE_ASSIGNMENT (token);
  char *varname;

  if (VIVI_IS_CODE_CONSTANT (assignment->name)) {
    varname = vivi_code_constant_get_variable_name (VIVI_CODE_CONSTANT (assignment->name));
  } else {
    varname = NULL;
  }

  if (assignment->from) {
    vivi_code_printer_print_value (printer, assignment->from, VIVI_PRECEDENCE_MEMBER);
    if (varname) {
      vivi_code_printer_print (printer, ".");
      vivi_code_printer_print (printer, varname);
    } else {
      vivi_code_printer_print (printer, "[");
      vivi_code_printer_print_value (printer, assignment->name, VIVI_PRECEDENCE_MIN);
      vivi_code_printer_print (printer, "]");
    }
  } else {
    if (varname) {
      vivi_code_printer_print (printer, varname);
    } else {
      vivi_code_printer_print (printer, "set (");
      vivi_code_printer_print_value (printer, assignment->name, VIVI_PRECEDENCE_MIN);
      vivi_code_printer_print (printer, ", ");
      vivi_code_printer_print_value (printer, assignment->value, VIVI_PRECEDENCE_ASSIGNMENT);
      vivi_code_printer_print (printer, ")");
      return;
    }
  }

  vivi_code_printer_print (printer, " = ");
  vivi_code_printer_print_value (printer, assignment->value, VIVI_PRECEDENCE_ASSIGNMENT);

  g_free (varname);
}

static void
vivi_code_assignment_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeAssignment *assignment = VIVI_CODE_ASSIGNMENT (token);
  ViviCodeAsm *code;

  if (assignment->from)
    vivi_code_compiler_compile_value (compiler, assignment->from);
  vivi_code_compiler_compile_value (compiler, assignment->name);
  vivi_code_compiler_compile_value (compiler, assignment->value);

  if (assignment->from) {
    code = vivi_code_asm_set_member_new ();
  } else {
    code = vivi_code_asm_set_variable_new ();
  }
  vivi_code_compiler_take_code (compiler, code);
}

static void
vivi_code_assignment_compile_value (ViviCodeValue *value,
    ViviCodeCompiler *compiler)
{
  ViviCodeAssignment *assignment = VIVI_CODE_ASSIGNMENT (value);

  if (assignment->from)
    vivi_code_compiler_compile_value (compiler, assignment->from);
  vivi_code_compiler_compile_value (compiler, assignment->name);
  vivi_code_compiler_compile_value (compiler, assignment->value);


  if (assignment->from) {
    // TODO
    vivi_code_compiler_take_code (compiler, vivi_code_asm_set_member_new ());
    g_assert_not_reached ();
  } else {
    vivi_code_compiler_take_code (compiler,
	vivi_code_asm_push_duplicate_new ());
    vivi_code_compiler_take_code (compiler, vivi_code_asm_swap_new ());
    vivi_code_compiler_take_code (compiler, vivi_code_asm_set_variable_new ());
  }
}

static void
vivi_code_assignment_class_init (ViviCodeAssignmentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_assignment_dispose;

  token_class->print = vivi_code_assignment_print;
  token_class->compile = vivi_code_assignment_compile;

  value_class->compile_value = vivi_code_assignment_compile_value;
}

static void
vivi_code_assignment_init (ViviCodeAssignment *assignment)
{
}

ViviCodeValue *
vivi_code_assignment_new (ViviCodeValue *from, ViviCodeValue *name, ViviCodeValue *value)
{
  ViviCodeAssignment *assignment;

  g_return_val_if_fail (from == NULL || VIVI_IS_CODE_VALUE (from), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (name), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  assignment = g_object_new (VIVI_TYPE_CODE_ASSIGNMENT, NULL);
  if (from)
    assignment->from = g_object_ref (from);
  assignment->name = g_object_ref (name);
  assignment->value = g_object_ref (value);

  return VIVI_CODE_VALUE (assignment);
}

ViviCodeValue *
vivi_code_assignment_new_name (const char *name, ViviCodeValue *value)
{
  ViviCodeValue *constant, *result;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  constant = vivi_code_string_new (name);
  result = vivi_code_assignment_new (NULL, constant, value);
  g_object_unref (constant);

  return result;
}
