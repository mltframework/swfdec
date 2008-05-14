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

#include "vivi_code_builtin_call.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"

G_DEFINE_ABSTRACT_TYPE (ViviCodeBuiltinCall, vivi_code_builtin_call, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_builtin_call_print_value (ViviCodeValue *value,
    ViviCodePrinter *printer)
{
  ViviCodeBuiltinCallClass *klass = VIVI_CODE_BUILTIN_CALL_GET_CLASS (value);

  g_assert (klass->function_name != NULL);
  vivi_code_printer_print (printer, klass->function_name);
  vivi_code_printer_print (printer, " ()");
}

static void
vivi_code_builtin_call_compile_value (ViviCodeValue *value,
    ViviCodeCompiler *compiler)
{
  ViviCodeBuiltinCallClass *klass = VIVI_CODE_BUILTIN_CALL_GET_CLASS (value);

  g_assert (klass->asm_constructor != NULL);
  vivi_code_compiler_take_code (compiler, klass->asm_constructor ());
}

static void
vivi_code_builtin_call_class_init (ViviCodeBuiltinCallClass *klass)
{
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  value_class->print_value = vivi_code_builtin_call_print_value;
  value_class->compile_value = vivi_code_builtin_call_compile_value;
}

static void
vivi_code_builtin_call_init (ViviCodeBuiltinCall *stmt)
{
}
