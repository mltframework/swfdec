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
 * License along with this library; if void, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vivi_code_void.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm.h"
#include "vivi_code_asm_code_default.h"
#include "vivi_code_asm_push.h"

G_DEFINE_TYPE (ViviCodeVoid, vivi_code_void, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_void_dispose (GObject *object)
{
  ViviCodeVoid *void_ = VIVI_CODE_VOID (object);

  g_object_unref (void_->value);

  G_OBJECT_CLASS (vivi_code_void_parent_class)->dispose (object);
}

static void
vivi_code_void_print_value (ViviCodeValue *value, ViviCodePrinter *printer)
{
  ViviCodeVoid *void_ = VIVI_CODE_VOID (value);

  vivi_code_printer_print (printer, "void ");
  vivi_code_printer_print_value (printer, void_->value, VIVI_PRECEDENCE_UNARY);
}

static void
vivi_code_void_compile_value (ViviCodeValue *value, ViviCodeCompiler *compiler)
{
  ViviCodeVoid *void_ = VIVI_CODE_VOID (value);
  ViviCodeAsm *push;

  vivi_code_compiler_compile_value (compiler, void_->value);
  vivi_code_compiler_take_code (compiler, vivi_code_asm_pop_new ());

  push = vivi_code_asm_push_new ();
  vivi_code_asm_push_add_undefined (VIVI_CODE_ASM_PUSH (push));
  vivi_code_compiler_take_code (compiler, push);
}

static void
vivi_code_void_class_init (ViviCodeVoidClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_void_dispose;

  value_class->print_value = vivi_code_void_print_value;
  value_class->compile_value = vivi_code_void_compile_value;
}

static void
vivi_code_void_init (ViviCodeVoid *void_)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (void_);

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_UNARY);
}

ViviCodeValue *
vivi_code_void_new (ViviCodeValue *value)
{
  ViviCodeVoid *void_;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  void_ = g_object_new (VIVI_TYPE_CODE_VOID, NULL);
  void_->value = g_object_ref (value);

  return VIVI_CODE_VALUE (void_);
}

ViviCodeValue *
vivi_code_void_get_value (ViviCodeVoid *void_)
{
  g_return_val_if_fail (VIVI_IS_CODE_VOID (void_), NULL);

  return void_->value;
}
