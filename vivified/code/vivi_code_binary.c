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

static struct {
  const char *		operation;
  guint			bytecode;
  ViviPrecedence	precedence;
} operation_table[] = {
  { "add",	SWFDEC_AS_ACTION_ADD,		VIVI_PRECEDENCE_ADD },
  { "-",	SWFDEC_AS_ACTION_SUBTRACT,	VIVI_PRECEDENCE_ADD },
  { "*",	SWFDEC_AS_ACTION_MULTIPLY,	VIVI_PRECEDENCE_MULTIPLY },
  { "/",	SWFDEC_AS_ACTION_DIVIDE,	VIVI_PRECEDENCE_MULTIPLY },
  { "==",	SWFDEC_AS_ACTION_EQUALS,	VIVI_PRECEDENCE_EQUALITY },
  { "<",	SWFDEC_AS_ACTION_LESS,		VIVI_PRECEDENCE_RELATIONAL },
  { "and",	SWFDEC_AS_ACTION_AND,		VIVI_PRECEDENCE_AND },
  { "or",	SWFDEC_AS_ACTION_OR,		VIVI_PRECEDENCE_OR },
  { "eq",	SWFDEC_AS_ACTION_STRING_EQUALS,	VIVI_PRECEDENCE_EQUALITY },
  { "lt",	SWFDEC_AS_ACTION_STRING_LESS,	VIVI_PRECEDENCE_RELATIONAL },
  { "+",	SWFDEC_AS_ACTION_ADD2,		VIVI_PRECEDENCE_ADD },
  { "<",	SWFDEC_AS_ACTION_LESS2,		VIVI_PRECEDENCE_RELATIONAL },
  { "==",	SWFDEC_AS_ACTION_EQUALS2,	VIVI_PRECEDENCE_EQUALITY },
  { "&",	SWFDEC_AS_ACTION_BIT_AND,	VIVI_PRECEDENCE_BINARY_AND },
  { "|",	SWFDEC_AS_ACTION_BIT_OR,	VIVI_PRECEDENCE_BINARY_OR },
  { "^",	SWFDEC_AS_ACTION_BIT_XOR,	VIVI_PRECEDENCE_BINARY_XOR },
  { "<<",	SWFDEC_AS_ACTION_BIT_LSHIFT,	VIVI_PRECEDENCE_SHIFT },
  { ">>",	SWFDEC_AS_ACTION_BIT_RSHIFT,	VIVI_PRECEDENCE_SHIFT },
  { ">>>",	SWFDEC_AS_ACTION_BIT_URSHIFT,	VIVI_PRECEDENCE_SHIFT },
  { "===",	SWFDEC_AS_ACTION_STRICT_EQUALS,	VIVI_PRECEDENCE_EQUALITY },
  { ">",	SWFDEC_AS_ACTION_GREATER,	VIVI_PRECEDENCE_RELATIONAL },
  { "gt",	SWFDEC_AS_ACTION_STRING_GREATER,VIVI_PRECEDENCE_RELATIONAL }
};

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

  vivi_code_printer_print_value (printer, binary->left, 
      operation_table[binary->operation_index].precedence);
  vivi_code_printer_print (printer, " ");
  vivi_code_printer_print (printer, operation_table[binary->operation_index].operation);
  vivi_code_printer_print (printer, " ");
  vivi_code_printer_print_value (printer, binary->right, 
      operation_table[binary->operation_index].precedence);
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

  value_class->is_constant = vivi_code_binary_is_constant;
}

static void
vivi_code_binary_init (ViviCodeBinary *binary)
{
}

ViviCodeValue *
vivi_code_binary_new_bytecode (ViviCodeValue *left, ViviCodeValue *right, guint code)
{
  ViviCodeBinary *binary;
  guint id;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (left), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (right), NULL);
  for (id = 0; id < G_N_ELEMENTS (operation_table); id++) {
    if (operation_table[id].bytecode == code)
      break;
  }
  g_return_val_if_fail (id < G_N_ELEMENTS (operation_table), NULL);

  binary = g_object_new (VIVI_TYPE_CODE_BINARY, NULL);
  binary->left = g_object_ref (left);
  binary->right = g_object_ref (right);
  binary->operation_index = id;

  vivi_code_value_set_precedence (VIVI_CODE_VALUE (binary), 
      operation_table[id].precedence);

  return VIVI_CODE_VALUE (binary);
}

