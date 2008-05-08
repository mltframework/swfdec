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

#include "vivi_code_unary.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeUnary, vivi_code_unary, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_unary_dispose (GObject *object)
{
  ViviCodeUnary *unary = VIVI_CODE_UNARY (object);

  g_object_unref (unary->value);

  G_OBJECT_CLASS (vivi_code_unary_parent_class)->dispose (object);
}

static ViviCodeValue * 
vivi_code_unary_optimize (ViviCodeValue *value, SwfdecAsValueType hint)
{
  ViviCodeUnary *unary = VIVI_CODE_UNARY (value);

  if (unary->operation == '!' && 
      hint == SWFDEC_AS_TYPE_BOOLEAN &&
      VIVI_IS_CODE_UNARY (unary->value) && 
      VIVI_CODE_UNARY (unary->value)->operation == unary->operation) {
    return vivi_code_value_optimize (VIVI_CODE_UNARY (unary->value)->value, hint);
  }

  /* FIXME: optimize unary->value */
  return g_object_ref (unary);
}

static void
vivi_code_unary_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeUnary *unary = VIVI_CODE_UNARY (token);
  char *sign;

  /* FIXME: ugly! */
  sign = g_strndup (&unary->operation, 1);
  vivi_code_printer_print (printer, sign);
  g_free (sign);

  vivi_code_printer_print_value (printer, unary->value, VIVI_PRECEDENCE_UNARY);
}

static void
vivi_code_unary_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeUnary *unary = VIVI_CODE_UNARY (token);
  ViviCodeAsm *code;

  g_return_if_fail (unary->operation != '!');

  vivi_code_compiler_compile_value (compiler, unary->value);

  code = vivi_code_asm_not_new ();
  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);
}

static gboolean
vivi_code_unary_is_constant (ViviCodeValue *value)
{
  return vivi_code_value_is_constant (VIVI_CODE_UNARY (value)->value);
}

static void
vivi_code_unary_class_init (ViviCodeUnaryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_unary_dispose;

  token_class->print = vivi_code_unary_print;
  token_class->compile = vivi_code_unary_compile;

  value_class->is_constant = vivi_code_unary_is_constant;
  value_class->optimize = vivi_code_unary_optimize;
}

static void
vivi_code_unary_init (ViviCodeUnary *unary)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (unary);

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_UNARY);
}

ViviCodeValue *
vivi_code_unary_new (ViviCodeValue *value, char operation)
{
  ViviCodeUnary *unary;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  unary = g_object_new (VIVI_TYPE_CODE_UNARY, NULL);
  unary->value = g_object_ref (value);
  unary->operation = operation;

  return VIVI_CODE_VALUE (unary);
}

ViviCodeValue *
vivi_code_unary_get_value (ViviCodeUnary *unary)
{
  g_return_val_if_fail (VIVI_IS_CODE_UNARY (unary), NULL);

  return unary->value;
}

