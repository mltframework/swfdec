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

#include <swfdec/swfdec_script_internal.h>

#include "vivi_code_function.h"
#include "vivi_code_printer.h"

#include "vivi_decompiler.h"

G_DEFINE_TYPE (ViviCodeFunction, vivi_code_function, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_function_dispose (GObject *object)
{
  ViviCodeFunction *function = VIVI_CODE_FUNCTION (object);

  swfdec_script_unref (function->script);
  if (function->body)
    g_object_unref (function->body);

  G_OBJECT_CLASS (vivi_code_function_parent_class)->dispose (object);
}

static void
vivi_code_function_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeFunction *function = VIVI_CODE_FUNCTION (token);
  guint i;

  vivi_code_printer_print (printer, "function (");
  for (i = 0; i < function->script->n_arguments; i++) {
    if (i != 0)
      vivi_code_printer_print (printer, ", ");
    vivi_code_printer_print (printer, function->script->arguments[i].name);
  }
  vivi_code_printer_print (printer, ") {");
  vivi_code_printer_new_line (printer, FALSE);
  if (function->body) {
    vivi_code_printer_push_indentation (printer);
    vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (function->body));
    vivi_code_printer_pop_indentation (printer);
  }
  vivi_code_printer_print (printer, "}");
  vivi_code_printer_new_line (printer, FALSE);
}

static gboolean
vivi_code_function_is_constant (ViviCodeValue *value)
{
  return TRUE;
}

static void
vivi_code_function_class_init (ViviCodeFunctionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_function_dispose;

  token_class->print = vivi_code_function_print;

  value_class->is_constant = vivi_code_function_is_constant;
}

static void
vivi_code_function_init (ViviCodeFunction *function)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (function);

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_CALL);
}

ViviCodeValue *
vivi_code_function_new (SwfdecScript *script)
{
  ViviCodeFunction *function;

  g_return_val_if_fail (script != NULL, NULL);

  function = g_object_new (VIVI_TYPE_CODE_FUNCTION, NULL);
  function->script = swfdec_script_ref (script);
  function->body = vivi_decompile_script (script);

  return VIVI_CODE_VALUE (function);
}

