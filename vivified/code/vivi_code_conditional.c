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

#include "vivi_code_conditional.h"
#include "vivi_code_get.h"
#include "vivi_code_printer.h"
#include "vivi_code_string.h"
#include "vivi_code_undefined.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_code_default.h"
#include "vivi_code_asm_if.h"
#include "vivi_code_asm_jump.h"

G_DEFINE_TYPE (ViviCodeConditional, vivi_code_conditional, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_conditional_dispose (GObject *object)
{
  ViviCodeConditional *conditional = VIVI_CODE_CONDITIONAL (object);

  g_object_unref (conditional->condition);
  g_object_unref (conditional->if_value);
  g_object_unref (conditional->else_value);

  G_OBJECT_CLASS (vivi_code_conditional_parent_class)->dispose (object);
}

static void
vivi_code_conditional_print_value (ViviCodeValue *value,
    ViviCodePrinter *printer)
{
  ViviCodeConditional *conditional = VIVI_CODE_CONDITIONAL (value);

  vivi_code_printer_print_value (printer, conditional->condition,
      VIVI_PRECEDENCE_CONDITIONAL);
  vivi_code_printer_print (printer, " ? ");
  vivi_code_printer_print_value (printer, conditional->if_value,
      VIVI_PRECEDENCE_CONDITIONAL);
  vivi_code_printer_print (printer, " : ");
  vivi_code_printer_print_value (printer, conditional->else_value,
      VIVI_PRECEDENCE_CONDITIONAL);
}

static void
vivi_code_conditional_compile_value (ViviCodeValue *value,
    ViviCodeCompiler *compiler)
{
  ViviCodeConditional *conditional = VIVI_CODE_CONDITIONAL (value);
  ViviCodeLabel *label_if, *label_end;

  vivi_code_compiler_compile_value (compiler, conditional->condition);

  label_if = vivi_code_compiler_create_label (compiler, "conditional_if");
  vivi_code_compiler_take_code (compiler, vivi_code_asm_if_new (label_if));

  vivi_code_compiler_compile_value (compiler, conditional->else_value);
  label_end = vivi_code_compiler_create_label (compiler, "conditional_end");
  vivi_code_compiler_take_code (compiler, vivi_code_asm_jump_new (label_end));

  vivi_code_compiler_take_code (compiler, VIVI_CODE_ASM (label_if));
  vivi_code_compiler_compile_value (compiler, conditional->if_value);

  vivi_code_compiler_take_code (compiler, VIVI_CODE_ASM (label_end));
}

static void
vivi_code_conditional_class_init (ViviCodeConditionalClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_conditional_dispose;

  value_class->print_value = vivi_code_conditional_print_value;
  value_class->compile_value = vivi_code_conditional_compile_value;
}

static void
vivi_code_conditional_init (ViviCodeConditional *conditional)
{
}

ViviCodeValue *
vivi_code_conditional_new (ViviCodeValue *condition, ViviCodeValue *if_value,
    ViviCodeValue *else_value)
{
  ViviCodeConditional *conditional;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (condition), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (if_value), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (else_value), NULL);

  conditional = g_object_new (VIVI_TYPE_CODE_CONDITIONAL, NULL);
  if (conditional)
    conditional->condition = g_object_ref (condition);
  conditional->if_value = g_object_ref (if_value);
  conditional->else_value = g_object_ref (else_value);

  return VIVI_CODE_VALUE (conditional);
}
