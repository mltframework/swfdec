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

#include "vivi_code_inc_dec.h"
#include "vivi_code_printer.h"
#include "vivi_code_constant.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_code_default.h"
#include "vivi_code_asm_push.h"
#include "vivi_code_asm_store.h"

G_DEFINE_TYPE (ViviCodeIncDec, vivi_code_inc_dec, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_inc_dec_dispose (GObject *object)
{
  ViviCodeIncDec *inc_dec = VIVI_CODE_INC_DEC (object);

  g_object_unref (inc_dec->name);

  G_OBJECT_CLASS (vivi_code_inc_dec_parent_class)->dispose (object);
}

static void
vivi_code_inc_dec_print_value (ViviCodeValue *value,
    ViviCodePrinter *printer)
{
  ViviCodeIncDec *inc_dec = VIVI_CODE_INC_DEC (value);
  char *varname;

  if (inc_dec->pre_assignment) {
    if (inc_dec->increment) {
      vivi_code_printer_print (printer, "++");
    } else {
      vivi_code_printer_print (printer, "--");
    }
  }

  // FIXME this is code duplication from ViviCodeAssignment
  if (VIVI_IS_CODE_CONSTANT (inc_dec->name)) {
    varname = vivi_code_constant_get_variable_name (
	VIVI_CODE_CONSTANT (inc_dec->name));
  } else {
    varname = NULL;
  }

  if (inc_dec->from) {
    vivi_code_printer_print_value (printer, inc_dec->from,
	VIVI_PRECEDENCE_MEMBER);
    if (varname) {
      vivi_code_printer_print (printer, ".");
      vivi_code_printer_print (printer, varname);
    } else {
      vivi_code_printer_print (printer, "[");
      vivi_code_printer_print_value (printer, inc_dec->name,
	  VIVI_PRECEDENCE_MIN);
      vivi_code_printer_print (printer, "]");
    }
  } else {
    if (varname) {
      vivi_code_printer_print (printer, varname);
    } else {
      // FIXME
      g_assert_not_reached ();
      return;
    }
  }

  if (!inc_dec->pre_assignment) {
    if (inc_dec->increment) {
      vivi_code_printer_print (printer, "++");
    } else {
      vivi_code_printer_print (printer, "--");
    }
  }
}

static void
vivi_code_inc_dec_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeIncDec *inc_dec = VIVI_CODE_INC_DEC (token);

  if (inc_dec->from) {
    vivi_code_compiler_compile_value (compiler, inc_dec->from);
    vivi_code_compiler_take_code (compiler, vivi_code_asm_get_variable_new ());
    vivi_code_compiler_take_code (compiler,
	vivi_code_asm_push_duplicate_new ());
    vivi_code_compiler_compile_value (compiler, inc_dec->name);
    vivi_code_compiler_take_code (compiler, vivi_code_asm_get_member_new ());
  } else {
    vivi_code_compiler_compile_value (compiler, inc_dec->name);
    vivi_code_compiler_take_code (compiler,
	vivi_code_asm_push_duplicate_new ());
    vivi_code_compiler_take_code (compiler, vivi_code_asm_get_variable_new ());
  }

  if (inc_dec->increment) {
    vivi_code_compiler_take_code (compiler, vivi_code_asm_increment_new ());
  } else {
    vivi_code_compiler_take_code (compiler, vivi_code_asm_decrement_new ());
  }

  if (inc_dec->from) {
    vivi_code_compiler_take_code (compiler, vivi_code_asm_swap_new ());
    vivi_code_compiler_take_code (compiler, vivi_code_asm_set_member_new ());
  } else {
    vivi_code_compiler_take_code (compiler, vivi_code_asm_set_variable_new ());
  }
}

static void
vivi_code_inc_dec_compile_value (ViviCodeValue *value,
    ViviCodeCompiler *compiler)
{
  ViviCodeIncDec *inc_dec = VIVI_CODE_INC_DEC (value);
  ViviCodeAsm *push;

  if (inc_dec->from) {
    vivi_code_compiler_compile_value (compiler, inc_dec->from);
    vivi_code_compiler_take_code (compiler, vivi_code_asm_get_variable_new ());
    vivi_code_compiler_take_code (compiler,
	vivi_code_asm_push_duplicate_new ());
    vivi_code_compiler_compile_value (compiler, inc_dec->name);
    vivi_code_compiler_take_code (compiler, vivi_code_asm_get_member_new ());
  } else {
    vivi_code_compiler_compile_value (compiler, inc_dec->name);
    vivi_code_compiler_take_code (compiler,
	vivi_code_asm_push_duplicate_new ());
    vivi_code_compiler_take_code (compiler, vivi_code_asm_get_variable_new ());
  }

  if (!inc_dec->pre_assignment)
    vivi_code_compiler_take_code (compiler, vivi_code_asm_store_new (0));

  if (inc_dec->increment) {
    vivi_code_compiler_take_code (compiler, vivi_code_asm_increment_new ());
  } else {
    vivi_code_compiler_take_code (compiler, vivi_code_asm_decrement_new ());
  }

  if (inc_dec->pre_assignment)
    vivi_code_compiler_take_code (compiler, vivi_code_asm_store_new (0));

  if (inc_dec->from) {
    vivi_code_compiler_take_code (compiler, vivi_code_asm_swap_new ());
    vivi_code_compiler_take_code (compiler, vivi_code_asm_set_member_new ());
  } else {
    vivi_code_compiler_take_code (compiler, vivi_code_asm_set_variable_new ());
  }

  push = vivi_code_asm_push_new ();
  vivi_code_asm_push_add_register (VIVI_CODE_ASM_PUSH (push), 0);
  vivi_code_compiler_take_code (compiler, push);
}

static void
vivi_code_inc_dec_class_init (ViviCodeIncDecClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_inc_dec_dispose;

  token_class->compile = vivi_code_inc_dec_compile;

  value_class->print_value = vivi_code_inc_dec_print_value;
  value_class->compile_value = vivi_code_inc_dec_compile_value;
}

static void
vivi_code_inc_dec_init (ViviCodeIncDec *inc_dec)
{
}

ViviCodeValue *
vivi_code_inc_dec_new (ViviCodeValue *from, ViviCodeValue *name,
    gboolean increment, gboolean pre_assignment)
{
  ViviCodeIncDec *inc_dec;

  g_return_val_if_fail (from == NULL || VIVI_IS_CODE_VALUE (from), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (name), NULL);

  inc_dec = g_object_new (VIVI_TYPE_CODE_INC_DEC, NULL);
  if (from != NULL)
    inc_dec->from = g_object_ref (from);
  inc_dec->name = g_object_ref (name);
  inc_dec->increment = increment;
  inc_dec->pre_assignment = pre_assignment;

  return VIVI_CODE_VALUE (inc_dec);
}
