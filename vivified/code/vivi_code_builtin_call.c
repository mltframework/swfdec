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
#include "vivi_code_assembler.h"

G_DEFINE_ABSTRACT_TYPE (ViviCodeBuiltinCall, vivi_code_builtin_call, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_builtin_call_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeBuiltinCallClass *klass = VIVI_CODE_BUILTIN_CALL_GET_CLASS (token);

  g_assert (klass->function_name != NULL);
  vivi_code_printer_print (printer, klass->function_name);
  vivi_code_printer_print (printer, " ();");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_builtin_call_compile (ViviCodeToken *token,
    ViviCodeAssembler *assembler)
{
  ViviCodeBuiltinCallClass *klass = VIVI_CODE_BUILTIN_CALL_GET_CLASS (token);
  ViviCodeAsm *code;

  g_assert (klass->asm_constructor != NULL);
  code = klass->asm_constructor ();
  vivi_code_assembler_add_code (assembler, code);
  g_object_unref (code);
}

static void
vivi_code_builtin_call_class_init (ViviCodeBuiltinCallClass *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  token_class->print = vivi_code_builtin_call_print;
  token_class->compile = vivi_code_builtin_call_compile;
}

static void
vivi_code_builtin_call_init (ViviCodeBuiltinCall *stmt)
{
}
