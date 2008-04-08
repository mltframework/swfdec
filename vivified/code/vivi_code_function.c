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
#include "vivi_code_constant.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"

#include "vivi_decompiler.h"

G_DEFINE_TYPE (ViviCodeFunction, vivi_code_function, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_function_dispose (GObject *object)
{
  ViviCodeFunction *function = VIVI_CODE_FUNCTION (object);
  guint i;

  if (function->body)
    g_object_unref (function->body);

  for (i = 0; i < function->arguments->len; i++) {
    g_free (g_ptr_array_index (function->arguments, i));
  }
  g_ptr_array_free (function->arguments, TRUE);

  G_OBJECT_CLASS (vivi_code_function_parent_class)->dispose (object);
}

static void
vivi_code_function_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeFunction *function = VIVI_CODE_FUNCTION (token);
  guint i;

  vivi_code_printer_print (printer, "function (");
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
vivi_code_function_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeFunction *function = VIVI_CODE_FUNCTION (token);
  guint i;

  vivi_code_compiler_begin_action (compiler, SWFDEC_AS_ACTION_DEFINE_FUNCTION2);

  vivi_code_compiler_write_string (compiler, "");
  vivi_code_compiler_write_u16 (compiler, function->arguments->len);
  vivi_code_compiler_write_u8 (compiler, 0);  // reg
  vivi_code_compiler_write_u16 (compiler, 0);  // flags

  // arguments
  for (i = 0; i < function->arguments->len; i++) {
    vivi_code_compiler_write_u8 (compiler, 0);  // register
    vivi_code_compiler_write_string (compiler,
	g_ptr_array_index (function->arguments, i )); // name
  }

  if (function->body != NULL) {
    vivi_code_compiler_compile_token (compiler,
	VIVI_CODE_TOKEN (function->body));
  }

  vivi_code_compiler_write_u16 (compiler,
      vivi_code_compiler_tail_size (compiler)); // body length

  vivi_code_compiler_end_action (compiler);
}

static gboolean
vivi_code_function_is_constant (ViviCodeValue *value)
{
  return FALSE;
}

static void
vivi_code_function_class_init (ViviCodeFunctionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_function_dispose;

  token_class->print = vivi_code_function_print;
  token_class->compile = vivi_code_function_compile;

  value_class->is_constant = vivi_code_function_is_constant;
}

static void
vivi_code_function_init (ViviCodeFunction *function)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (function);

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_CALL);

  function->arguments = g_ptr_array_new ();
}

ViviCodeValue *
vivi_code_function_new (void)
{
  return VIVI_CODE_VALUE (g_object_new (VIVI_TYPE_CODE_FUNCTION, NULL));
}

void
vivi_code_function_set_body (ViviCodeFunction *function,
    ViviCodeStatement *body)
{
  g_return_if_fail (VIVI_IS_CODE_FUNCTION (function));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (body));

  function->body = g_object_ref (body);
}

void
vivi_code_function_add_argument (ViviCodeFunction *function, const char *name)
{
  g_return_if_fail (VIVI_IS_CODE_FUNCTION (function));
  g_return_if_fail (name != NULL);

  g_ptr_array_add (function->arguments, g_strdup (name));
}

ViviCodeValue *
vivi_code_function_new_from_script (SwfdecScript *script)
{
  ViviCodeFunction *function;
  guint i;

  g_return_val_if_fail (script != NULL, NULL);

  function = g_object_new (VIVI_TYPE_CODE_FUNCTION, NULL);
  function->body = vivi_decompile_script (script);

  for (i = 0; i < script->n_arguments; i++) {
    vivi_code_function_add_argument (function, script->arguments[i].name);
  }

  return VIVI_CODE_VALUE (function);
}

