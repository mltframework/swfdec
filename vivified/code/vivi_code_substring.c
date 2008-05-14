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

#include "vivi_code_substring.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"

G_DEFINE_TYPE (ViviCodeSubstring, vivi_code_substring, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_substring_dispose (GObject *object)
{
  ViviCodeSubstring *substring = VIVI_CODE_SUBSTRING (object);

  g_object_unref (substring->string);
  g_object_unref (substring->index_);
  g_object_unref (substring->count);

  G_OBJECT_CLASS (vivi_code_substring_parent_class)->dispose (object);
}

static void
vivi_code_substring_print_value (ViviCodeValue *value,
    ViviCodePrinter *printer)
{
  ViviCodeSubstring *substring = VIVI_CODE_SUBSTRING (value);

  vivi_code_printer_print (printer, "substring (");
  vivi_code_printer_print_value (printer, substring->string,
      VIVI_PRECEDENCE_COMMA);
  vivi_code_printer_print (printer, ", ");
  vivi_code_printer_print_value (printer, substring->index_,
      VIVI_PRECEDENCE_COMMA);
  vivi_code_printer_print (printer, ", ");
  vivi_code_printer_print_value (printer, substring->count,
      VIVI_PRECEDENCE_COMMA);
  vivi_code_printer_print (printer, ")");
}

static void
vivi_code_substring_compile_value (ViviCodeValue *value,
    ViviCodeCompiler *compiler)
{
  g_printerr ("Implement substring\n");
#if 0
  ViviCodeSubstring *substring = VIVI_CODE_SUBSTRING (value);

  vivi_code_value_compile (substring->string, compiler);
  vivi_code_value_compile (substring->index_, compiler);
  vivi_code_value_compile (substring->count, compiler);

  vivi_code_compiler_take_code (compiler, vivi_code_asm_string_extract_new ());
#endif
}

static void
vivi_code_substring_class_init (ViviCodeSubstringClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_substring_dispose;

  value_class->print_value = vivi_code_substring_print_value;
  value_class->compile_value = vivi_code_substring_compile_value;
}

static void
vivi_code_substring_init (ViviCodeSubstring *token)
{
}

ViviCodeValue *
vivi_code_substring_new (ViviCodeValue *string, ViviCodeValue *index_,
    ViviCodeValue *count)
{
  ViviCodeSubstring *ret;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (string), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (index_), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (count), NULL);

  ret = g_object_new (VIVI_TYPE_CODE_SUBSTRING, NULL);
  ret->string = g_object_ref (string);
  ret->index_ = g_object_ref (index_);
  ret->count = g_object_ref (count);

  return VIVI_CODE_VALUE (ret);
}
