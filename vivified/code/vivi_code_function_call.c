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

#include "vivi_code_function_call.h"
#include "vivi_code_constant.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCodeFunctionCall, vivi_code_function_call, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_function_call_dispose (GObject *object)
{
  ViviCodeFunctionCall *call = VIVI_CODE_FUNCTION_CALL (object);

  if (call->value)
    g_object_unref (call->value);
  if (call->name)
    g_object_unref (call->name);
  g_ptr_array_foreach (call->arguments, (GFunc) g_object_unref, NULL);
  g_ptr_array_free (call->arguments, TRUE);

  G_OBJECT_CLASS (vivi_code_function_call_parent_class)->dispose (object);
}

static ViviCodeValue * 
vivi_code_function_call_optimize (ViviCodeValue *value, SwfdecAsValueType hint)
{
  ViviCodeFunctionCall *call = VIVI_CODE_FUNCTION_CALL (value);
  ViviCodeValue *opt, *name;
  guint i;

  if (call->value)
    value = vivi_code_value_optimize (call->value, SWFDEC_AS_TYPE_UNDEFINED);
  else
    value = NULL;
  if (call->name)
    name = vivi_code_value_optimize (call->name, SWFDEC_AS_TYPE_UNDEFINED);
  else
    name = NULL;

  opt = vivi_code_function_call_new (value, name);
  if (value)
    g_object_unref (value);
  if (name)
    g_object_unref (name);
  for (i = 0; i < call->arguments->len; i++) {
    value = vivi_code_value_optimize (g_ptr_array_index (call->arguments, i),
	SWFDEC_AS_TYPE_UNDEFINED);
    vivi_code_function_call_add_argument (VIVI_CODE_FUNCTION_CALL (opt), value);
    g_object_unref (value);
  }
  return opt;
}

static void
vivi_code_function_call_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeFunctionCall *call = VIVI_CODE_FUNCTION_CALL (token);
  char *varname;
  guint i;

  if (call->name) {
    if (VIVI_IS_CODE_CONSTANT (call->name))
      varname = vivi_code_constant_get_variable_name (VIVI_CODE_CONSTANT (call->name));
    else
      varname = NULL;

    if (call->value) {
      vivi_code_printer_print_value (printer, call->value, VIVI_PRECEDENCE_MEMBER);
      if (varname) {
	vivi_code_printer_print (printer, ".");
	vivi_code_printer_print (printer, varname);
      } else {
	vivi_code_printer_print (printer, "[");
	vivi_code_printer_print_value (printer, call->name, VIVI_PRECEDENCE_MIN);
	vivi_code_printer_print (printer, "]");
      }
    } else {
      if (varname) {
	vivi_code_printer_print (printer, varname);
      } else {
	vivi_code_printer_print (printer, "eval (");
	vivi_code_printer_print_value (printer, call->name, VIVI_PRECEDENCE_MIN);
	vivi_code_printer_print (printer, ")");
      }
    }
    g_free (varname);
  } else {
    vivi_code_printer_print_value (printer, call->value, VIVI_PRECEDENCE_MEMBER);
  }
  vivi_code_printer_print (printer, " (");
  for (i = 0; i < call->arguments->len; i++) {
    if (i > 0)
      vivi_code_printer_print (printer, ", ");
    vivi_code_printer_print_value (printer, g_ptr_array_index (call->arguments, i), VIVI_PRECEDENCE_COMMA);
  }
  vivi_code_printer_print (printer, ")");
}

static gboolean
vivi_code_function_call_is_constant (ViviCodeValue *value)
{
  return FALSE;
}

static void
vivi_code_function_call_class_init (ViviCodeFunctionCallClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_function_call_dispose;

  token_class->print = vivi_code_function_call_print;

  value_class->is_constant = vivi_code_function_call_is_constant;
  value_class->optimize = vivi_code_function_call_optimize;
}

static void
vivi_code_function_call_init (ViviCodeFunctionCall *call)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (call);

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_CALL);

  call->arguments = g_ptr_array_new ();
}

ViviCodeValue *
vivi_code_function_call_new (ViviCodeValue *value, ViviCodeValue *name)
{
  ViviCodeFunctionCall *call;

  g_return_val_if_fail (value != NULL || name != NULL, NULL);
  g_return_val_if_fail (value == NULL || VIVI_IS_CODE_VALUE (value), NULL);
  g_return_val_if_fail (name == NULL || VIVI_IS_CODE_VALUE (name), NULL);

  call = g_object_new (VIVI_TYPE_CODE_FUNCTION_CALL, NULL);
  if (value)
    call->value = g_object_ref (value);
  if (name)
    call->name = g_object_ref (name);

  return VIVI_CODE_VALUE (call);
}

void
vivi_code_function_call_add_argument (ViviCodeFunctionCall *call, ViviCodeValue *argument)
{
  g_return_if_fail (VIVI_IS_CODE_FUNCTION_CALL (call));
  g_return_if_fail (VIVI_IS_CODE_VALUE (argument));

  g_ptr_array_add (call->arguments, g_object_ref (argument));
}

