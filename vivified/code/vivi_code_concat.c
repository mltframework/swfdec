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

#include "vivi_code_concat.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeConcat, vivi_code_concat, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_concat_dispose (GObject *object)
{
  ViviCodeConcat *concat = VIVI_CODE_CONCAT (object);

  g_object_unref (concat->first);
  g_object_unref (concat->second);

  G_OBJECT_CLASS (vivi_code_concat_parent_class)->dispose (object);
}

static void
vivi_code_concat_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeConcat *concat = VIVI_CODE_CONCAT (token);

  vivi_code_printer_print (printer, "concat (");
  vivi_code_printer_print_value (printer, concat->first,
      VIVI_PRECEDENCE_COMMA);
  vivi_code_printer_print (printer, ", ");
  vivi_code_printer_print_value (printer, concat->second,
      VIVI_PRECEDENCE_COMMA);
  vivi_code_printer_print (printer, ");");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_concat_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeConcat *concat = VIVI_CODE_CONCAT (token);
  ViviCodeAsm *code;

  vivi_code_compiler_compile_value (compiler, concat->first);
  vivi_code_compiler_compile_value (compiler, concat->second);

  code = vivi_code_asm_string_add_new ();
  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);
}

static void
vivi_code_concat_class_init (ViviCodeConcatClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_concat_dispose;

  token_class->print = vivi_code_concat_print;
  token_class->compile = vivi_code_concat_compile;
}

static void
vivi_code_concat_init (ViviCodeConcat *token)
{
}

ViviCodeValue *
vivi_code_concat_new (ViviCodeValue *first, ViviCodeValue *second)
{
  ViviCodeConcat *ret;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (first), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (second), NULL);

  ret = g_object_new (VIVI_TYPE_CODE_CONCAT, NULL);
  ret->first = g_object_ref (first);
  ret->second = g_object_ref (second);

  return VIVI_CODE_VALUE (ret);
}
