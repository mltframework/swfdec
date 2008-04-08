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
#include "vivi_code_constant.h"
#include "vivi_code_get.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"

G_DEFINE_TYPE (ViviCodeAssignment, vivi_code_assignment, VIVI_TYPE_CODE_STATEMENT)

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

  if (assignment->local)
    vivi_code_printer_print (printer, "var ");

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
      goto finalize;
    }
  }
  if (!assignment->local || !VIVI_IS_CODE_CONSTANT (assignment->value) ||
      vivi_code_constant_get_value_type (
	VIVI_CODE_CONSTANT (assignment->value)) != SWFDEC_AS_TYPE_UNDEFINED) {
    vivi_code_printer_print (printer, " = ");
    vivi_code_printer_print_value (printer, assignment->value, VIVI_PRECEDENCE_ASSIGNMENT);
  }
  g_free (varname);
finalize:
  vivi_code_printer_print (printer, ";");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_assignment_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeAssignment *assignment = VIVI_CODE_ASSIGNMENT (token);

  if (assignment->local) {
    vivi_code_compiler_add_action (compiler, SWFDEC_AS_ACTION_DEFINE_LOCAL);
  } else if (assignment->from) {
    vivi_code_compiler_add_action (compiler, SWFDEC_AS_ACTION_SET_MEMBER);
  } else {
    vivi_code_compiler_add_action (compiler, SWFDEC_AS_ACTION_SET_VARIABLE);
  }

  if (assignment->local && assignment->from) {
    ViviCodeValue *get =
      vivi_code_get_new (assignment->from, assignment->value);
    vivi_code_compiler_compile_value (compiler, get);
    g_object_unref (get);
  } else {
    vivi_code_compiler_compile_value (compiler, assignment->value);
  }

  vivi_code_compiler_compile_value (compiler, assignment->name);
  if (assignment->from && !assignment->local)
    vivi_code_compiler_compile_value (compiler, assignment->from);

  vivi_code_compiler_end_action (compiler);
}

static void
vivi_code_assignment_class_init (ViviCodeAssignmentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_assignment_dispose;

  token_class->print = vivi_code_assignment_print;
  token_class->compile = vivi_code_assignment_compile;
}

static void
vivi_code_assignment_init (ViviCodeAssignment *assignment)
{
}

ViviCodeStatement *
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

  return VIVI_CODE_STATEMENT (assignment);
}

ViviCodeStatement *
vivi_code_assignment_new_name (const char *name, ViviCodeValue *value)
{
  ViviCodeValue *constant;
  ViviCodeStatement *result;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  constant = vivi_code_constant_new_string (name);
  result = vivi_code_assignment_new (NULL, constant, value);
  g_object_unref (constant);
  return result;
}

void
vivi_code_assignment_set_local (ViviCodeAssignment *assign, gboolean local)
{
  g_return_if_fail (VIVI_IS_CODE_ASSIGNMENT (assign));
  /* can't assign to non-local variables */
  g_return_if_fail (assign->from == NULL);

  assign->local = local;
}

