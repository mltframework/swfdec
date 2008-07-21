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

#include "vivi_code_compiler.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeCompiler, vivi_code_compiler, G_TYPE_OBJECT)

static void
vivi_code_compiler_dispose (GObject *object)
{
  ViviCodeCompiler *compiler = VIVI_CODE_COMPILER (object);

  g_assert (compiler->loop_labels == NULL);

  g_object_unref (compiler->assembler);

  G_OBJECT_CLASS (vivi_code_compiler_parent_class)->dispose (object);
}

static void
vivi_code_compiler_class_init (ViviCodeCompilerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_code_compiler_dispose;
}

static void
vivi_code_compiler_init (ViviCodeCompiler *compiler)
{
  compiler->assembler = VIVI_CODE_ASSEMBLER (vivi_code_assembler_new ());
}

void
vivi_code_compiler_compile_value (ViviCodeCompiler *compiler,
    ViviCodeValue *value)
{
  ViviCodeValueClass *klass;

  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (VIVI_IS_CODE_VALUE (value));

  klass = VIVI_CODE_VALUE_GET_CLASS (value);
  g_return_if_fail (klass->compile_value);
  klass->compile_value (value, compiler);
}

void
vivi_code_compiler_compile_token (ViviCodeCompiler *compiler,
    ViviCodeToken *token)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (VIVI_IS_CODE_TOKEN (token));

  vivi_code_token_compile (token, compiler);
}

void
vivi_code_compiler_compile_script (ViviCodeCompiler *compiler,
    ViviCodeStatement *statement)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (statement));

  vivi_code_compiler_compile_statement (compiler, statement);
  vivi_code_compiler_take_code (compiler, vivi_code_asm_end_new ());
}

void
vivi_code_compiler_add_code (ViviCodeCompiler *compiler, ViviCodeAsm *code)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (VIVI_IS_CODE_ASM (code));

  vivi_code_assembler_add_code (compiler->assembler, code);
}

void
vivi_code_compiler_take_code (ViviCodeCompiler *compiler, ViviCodeAsm *code)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (VIVI_IS_CODE_ASM (code));

  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);
}

ViviCodeLabel *
vivi_code_compiler_create_label (ViviCodeCompiler *compiler,
    const char *prefix)
{
  char *name;
  ViviCodeLabel *label;

  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), NULL);
  g_return_val_if_fail (prefix != NULL, NULL);

  compiler->num_labels++;

  name = g_strdup_printf ("%s_%04i", prefix, compiler->num_labels);
  label = VIVI_CODE_LABEL (vivi_code_label_new (name));
  g_free (name);

  return label;
}

typedef struct {
  ViviCodeLabel *	continue_label;
  ViviCodeLabel *	break_label;
  guint			continue_used;
  guint			break_used;
} LoopLabels;

void
vivi_code_compiler_push_loop_labels (ViviCodeCompiler *compiler,
    ViviCodeLabel *continue_label, ViviCodeLabel *break_label)
{
  LoopLabels *labels;

  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (continue_label == NULL ||
      VIVI_IS_CODE_LABEL (continue_label));
  g_return_if_fail (break_label == NULL || VIVI_IS_CODE_LABEL (break_label));

  if (continue_label != NULL)
      g_object_ref (continue_label);
  if (break_label != NULL)
      g_object_ref (break_label);

  labels = g_new0 (LoopLabels, 1);
  labels->continue_label = continue_label;
  labels->break_label = break_label;

  compiler->loop_labels = g_slist_prepend (compiler->loop_labels, labels);
}

void
vivi_code_compiler_pop_loop_labels (ViviCodeCompiler *compiler,
    guint *continue_used, guint *break_used)
{
  LoopLabels *labels;

  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (compiler->loop_labels != NULL);

  labels = compiler->loop_labels->data;

  if (continue_used != NULL)
    *continue_used = labels->continue_used;
  if (break_used != NULL)
    *break_used = labels->break_used;

  if (labels->continue_label != NULL)
    g_object_unref (labels->continue_label);
  if (labels->break_label != NULL)
    g_object_unref (labels->break_label);

  compiler->loop_labels =
    g_slist_delete_link (compiler->loop_labels, compiler->loop_labels);
}

ViviCodeLabel *
vivi_code_compiler_use_continue_label (ViviCodeCompiler *compiler)
{
  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), NULL);

  if (compiler->loop_labels == NULL)
    return NULL;

  ((LoopLabels *)compiler->loop_labels->data)->continue_used++;
  return VIVI_CODE_LABEL (
      ((LoopLabels *)compiler->loop_labels->data)->continue_label);
}

ViviCodeLabel *
vivi_code_compiler_use_break_label (ViviCodeCompiler *compiler)
{
  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), NULL);

  if (compiler->loop_labels == NULL)
    return NULL;

  ((LoopLabels *)compiler->loop_labels->data)->break_used++;
  return VIVI_CODE_LABEL (
      ((LoopLabels *)compiler->loop_labels->data)->break_label);
}

ViviCodeCompiler *
vivi_code_compiler_new (guint version)
{
  ViviCodeCompiler *compiler;

  compiler = g_object_new (VIVI_TYPE_CODE_COMPILER, NULL);
  compiler->version = version;

  return compiler;
}

guint
vivi_code_compiler_get_version (ViviCodeCompiler *compiler)
{
  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), 0);

  return compiler->version;
}


ViviCodeAssembler *
vivi_code_compiler_get_assembler (ViviCodeCompiler *compiler)
{
  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), NULL);

  return compiler->assembler;
}
