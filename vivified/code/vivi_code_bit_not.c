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
 * License along with this library; if bit_not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vivi_code_bit_not.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm.h"
#include "vivi_code_asm_code_default.h"
#include "vivi_code_asm_push.h"

G_DEFINE_TYPE (ViviCodeBitNot, vivi_code_bit_not, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_bit_not_dispose (GObject *object)
{
  ViviCodeBitNot *bit_not = VIVI_CODE_BIT_NOT (object);

  g_object_unref (bit_not->value);

  G_OBJECT_CLASS (vivi_code_bit_not_parent_class)->dispose (object);
}

static void
vivi_code_bit_not_print_value (ViviCodeValue *value, ViviCodePrinter *printer)
{
  ViviCodeBitNot *bit_not = VIVI_CODE_BIT_NOT (value);

  vivi_code_printer_print (printer, "~");
  vivi_code_printer_print_value (printer, bit_not->value, VIVI_PRECEDENCE_UNARY);
}

static void
vivi_code_bit_not_compile_value (ViviCodeValue *value, ViviCodeCompiler *compiler)
{
  ViviCodeBitNot *bit_not = VIVI_CODE_BIT_NOT (value);
  ViviCodeAsm *push;

  vivi_code_compiler_compile_value (compiler, bit_not->value);
  push = vivi_code_asm_push_new ();
  vivi_code_asm_push_add_integer (VIVI_CODE_ASM_PUSH (push), -1);
  vivi_code_compiler_take_code (compiler, push);
  vivi_code_compiler_take_code (compiler, vivi_code_asm_bit_xor_new ());
}

static void
vivi_code_bit_not_class_init (ViviCodeBitNotClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_bit_not_dispose;

  value_class->print_value = vivi_code_bit_not_print_value;
  value_class->compile_value = vivi_code_bit_not_compile_value;
}

static void
vivi_code_bit_not_init (ViviCodeBitNot *bit_not)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (bit_not);

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_UNARY);
}

ViviCodeValue *
vivi_code_bit_not_new (ViviCodeValue *value)
{
  ViviCodeBitNot *bit_not;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  bit_not = g_object_new (VIVI_TYPE_CODE_BIT_NOT, NULL);
  bit_not->value = g_object_ref (value);

  return VIVI_CODE_VALUE (bit_not);
}

ViviCodeValue *
vivi_code_bit_not_get_value (ViviCodeBitNot *bit_not)
{
  g_return_val_if_fail (VIVI_IS_CODE_BIT_NOT (bit_not), NULL);

  return bit_not->value;
}
