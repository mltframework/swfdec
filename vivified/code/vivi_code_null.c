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

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "vivi_code_null.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_push.h"

G_DEFINE_TYPE (ViviCodeNull, vivi_code_null, VIVI_TYPE_CODE_CONSTANT)

static void
vivi_code_null_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  vivi_code_printer_print (printer, "null");
}

static void
vivi_code_null_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeAsm *code;

  code = vivi_code_asm_push_new ();
  vivi_code_asm_push_add_null (VIVI_CODE_ASM_PUSH (code));
  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);
}

static char *
vivi_code_null_get_variable_name (ViviCodeConstant *constant)
{
  return g_strdup ("null");
}

static void
vivi_code_null_class_init (ViviCodeNullClass *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeConstantClass *constant_class = VIVI_CODE_CONSTANT_CLASS (klass);

  token_class->print = vivi_code_null_print;
  token_class->compile = vivi_code_null_compile;

  constant_class->get_variable_name = vivi_code_null_get_variable_name;
}

static void
vivi_code_null_init (ViviCodeNull *null)
{
}

ViviCodeValue *
vivi_code_null_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_NULL, NULL);
}

