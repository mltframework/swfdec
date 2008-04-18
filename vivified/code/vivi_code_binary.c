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

#include <swfdec/swfdec_as_interpret.h>

#include "vivi_code_binary.h"
#include "vivi_code_printer.h"
#include "vivi_code_assembler.h"

G_DEFINE_TYPE (ViviCodeBinary, vivi_code_binary, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_binary_dispose (GObject *object)
{
  ViviCodeBinary *binary = VIVI_CODE_BINARY (object);

  g_object_unref (binary->left);
  g_object_unref (binary->right);

  G_OBJECT_CLASS (vivi_code_binary_parent_class)->dispose (object);
}

static void
vivi_code_binary_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeBinary *binary = VIVI_CODE_BINARY (token);
  ViviCodeValue *value = VIVI_CODE_VALUE (token);
  ViviCodeBinaryClass *klass = VIVI_CODE_BINARY_GET_CLASS (binary);

  vivi_code_printer_print_value (printer, binary->left, 
      vivi_code_value_get_precedence (value));
  vivi_code_printer_print (printer, " ");
  vivi_code_printer_print (printer, klass->operator_name);
  vivi_code_printer_print (printer, " ");
  vivi_code_printer_print_value (printer, binary->right, 
      vivi_code_value_get_precedence (value));
}

static void
vivi_code_binary_compile (ViviCodeToken *token, ViviCodeAssembler *assembler)
{
  ViviCodeBinary *binary = VIVI_CODE_BINARY (token);
  ViviCodeBinaryClass *klass = VIVI_CODE_BINARY_GET_CLASS (binary);

  vivi_code_value_compile (binary->left, assembler);
  vivi_code_value_compile (binary->right, assembler);

  g_assert (klass->asm_constructor != 0);
  vivi_code_assembler_add_code (assembler, klass->asm_constructor ());
}

static gboolean
vivi_code_binary_is_constant (ViviCodeValue *value)
{
  ViviCodeBinary *binary = VIVI_CODE_BINARY (value);

  return vivi_code_value_is_constant (binary->left) &&
      vivi_code_value_is_constant (binary->right);
}

static void
vivi_code_binary_class_init (ViviCodeBinaryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_binary_dispose;

  token_class->print = vivi_code_binary_print;
  token_class->compile = vivi_code_binary_compile;

  value_class->is_constant = vivi_code_binary_is_constant;
}

static void
vivi_code_binary_init (ViviCodeBinary *binary)
{
}

