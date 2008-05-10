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

#include "vivi_code_and.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_code_default.h"
#include "vivi_code_asm_if.h"

G_DEFINE_TYPE (ViviCodeAnd, vivi_code_and, VIVI_TYPE_CODE_BINARY)

static void
vivi_code_and_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeBinary *binary = VIVI_CODE_BINARY (token);
  ViviCodeAsm *code;
  ViviCodeLabel *label_end;

  vivi_code_compiler_compile_value (compiler, binary->left);

  code = vivi_code_asm_push_duplicate_new ();
  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);

  code = vivi_code_asm_not_new ();
  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);

  label_end = vivi_code_compiler_create_label (compiler, "and_end");
  code = vivi_code_asm_if_new (label_end);
  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);

  code = vivi_code_asm_pop_new ();
  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);

  vivi_code_compiler_compile_value (compiler, binary->right);

  vivi_code_compiler_add_code (compiler, VIVI_CODE_ASM (label_end));
  g_object_unref (label_end);
}

static void
vivi_code_and_class_init (ViviCodeAndClass *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeBinaryClass *binary_class = VIVI_CODE_BINARY_CLASS (klass);

  token_class->compile = vivi_code_and_compile;

  binary_class->operator_name = "&&";
}

static void
vivi_code_and_init (ViviCodeAnd *and)
{
  vivi_code_value_set_precedence (VIVI_CODE_VALUE (and), VIVI_PRECEDENCE_AND);
}

ViviCodeValue *
vivi_code_and_new (ViviCodeValue *left, ViviCodeValue *right)
{
  ViviCodeBinary *binary;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (left), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (right), NULL);

  binary = g_object_new (VIVI_TYPE_CODE_AND, NULL);
  binary->left = g_object_ref (left);
  binary->right = g_object_ref (right);

  return VIVI_CODE_VALUE (binary);
}

