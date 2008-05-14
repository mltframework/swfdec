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

#include <string.h>

#include "vivi_code_string.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_push.h"

G_DEFINE_TYPE (ViviCodeString, vivi_code_string, VIVI_TYPE_CODE_CONSTANT)

static void
vivi_code_string_dispose (GObject *object)
{
  ViviCodeString *string = VIVI_CODE_STRING (object);

  g_free (string->value);
  string->value = NULL;

  G_OBJECT_CLASS (vivi_code_string_parent_class)->dispose (object);
}

static void
vivi_code_string_print_value (ViviCodeValue *value, ViviCodePrinter *printer)
{
  ViviCodeString *string = VIVI_CODE_STRING (value);
  char *s;

  s = vivi_code_escape_string (string->value);
  vivi_code_printer_print (printer, s);
  g_free (s);
}

static void
vivi_code_string_compile_value (ViviCodeValue *value,
    ViviCodeCompiler *compiler)
{
  ViviCodeString *string = VIVI_CODE_STRING (value);
  ViviCodeAsm *code;

  code = vivi_code_asm_push_new ();
  vivi_code_asm_push_add_string (VIVI_CODE_ASM_PUSH (code), string->value);
  vivi_code_compiler_take_code (compiler, code);
}

static char *
vivi_code_string_get_variable_name (ViviCodeConstant *constant)
{
  static const char *accept = G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "_$";
  ViviCodeString *string = VIVI_CODE_STRING (constant);
  gsize len;

  len = strspn (string->value, accept);
  if (string->value[len] == '\0')
    return g_strdup (string->value);
  else
    return NULL;
}

static void
vivi_code_string_class_init (ViviCodeStringClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);
  ViviCodeConstantClass *constant_class = VIVI_CODE_CONSTANT_CLASS (klass);

  object_class->dispose = vivi_code_string_dispose;

  value_class->print_value = vivi_code_string_print_value;
  value_class->compile_value = vivi_code_string_compile_value;

  constant_class->get_variable_name = vivi_code_string_get_variable_name;
}

static void
vivi_code_string_init (ViviCodeString *string)
{
}

ViviCodeValue *
vivi_code_string_new (const char *value)
{
  ViviCodeString *string;

  g_return_val_if_fail (value != NULL, NULL);

  string = g_object_new (VIVI_TYPE_CODE_STRING, NULL);
  string->value = g_strdup (value);

  return VIVI_CODE_VALUE (string);
}

const char *
vivi_code_string_get_value (ViviCodeString *string)
{
  g_return_val_if_fail (VIVI_IS_CODE_STRING (string), NULL);

  return string->value;
}

