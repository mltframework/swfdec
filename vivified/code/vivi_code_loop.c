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

#include "vivi_code_loop.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_label.h"
#include "vivi_code_asm_if.h"
#include "vivi_code_asm_jump.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeLoop, vivi_code_loop, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_loop_dispose (GObject *object)
{
  ViviCodeLoop *loop = VIVI_CODE_LOOP (object);

  if (loop->condition)
    g_object_unref (loop->condition);
  if (loop->statement)
    g_object_unref (loop->statement);

  G_OBJECT_CLASS (vivi_code_loop_parent_class)->dispose (object);
}

static void
vivi_code_loop_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeLoop *loop= VIVI_CODE_LOOP (token);
  gboolean needs_braces;

  needs_braces = loop->statement && vivi_code_statement_needs_braces (loop->statement);
  if (loop->condition) {
    vivi_code_printer_print (printer, "while (");
    vivi_code_printer_print_value (printer, loop->condition,
	VIVI_PRECEDENCE_MIN);
    vivi_code_printer_print (printer, ")");
  } else {
    vivi_code_printer_print (printer, "for (;;)");
  }
  if (needs_braces)
    vivi_code_printer_print (printer, " {");
  vivi_code_printer_new_line (printer, FALSE);
  vivi_code_printer_push_indentation (printer);
  if (loop->statement) {
    vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (loop->statement));
  } else {
    vivi_code_printer_print (printer, ";");
  }
  vivi_code_printer_pop_indentation (printer);
  if (needs_braces) {
    vivi_code_printer_print (printer, "}");
    vivi_code_printer_new_line (printer, FALSE);
  }
}

static void
vivi_code_loop_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeLoop *loop = VIVI_CODE_LOOP (token);
  ViviCodeLabel *label_start, *label_end = NULL;
  guint break_used;

  label_start = vivi_code_compiler_create_label (compiler, "loop_start");
  label_end = vivi_code_compiler_create_label (compiler, "loop_end");
  vivi_code_compiler_push_loop_labels (compiler, label_start, label_end);

  vivi_code_compiler_add_code (compiler, VIVI_CODE_ASM (label_start));

  if (loop->post_condition && loop->statement)
    vivi_code_compiler_compile_statement (compiler, loop->statement);

  if (loop->condition) {
    vivi_code_compiler_compile_value (compiler, loop->condition);

    if (!loop->post_condition) {
      vivi_code_compiler_take_code (compiler, vivi_code_asm_not_new ());
      vivi_code_compiler_take_code (compiler,
	  vivi_code_asm_if_new (label_end));
    }
  }

  if (!loop->post_condition && loop->statement)
    vivi_code_compiler_compile_statement (compiler, loop->statement);

  if (loop->post_condition) {
    vivi_code_compiler_take_code (compiler,
	vivi_code_asm_if_new (label_start));
  } else {
    vivi_code_compiler_take_code (compiler,
	vivi_code_asm_jump_new (label_start));
  }
  g_object_unref (label_start);

  vivi_code_compiler_pop_loop_labels (compiler, NULL, &break_used);

  if ((loop->condition && !loop->post_condition) || break_used > 0)
    vivi_code_compiler_add_code (compiler, VIVI_CODE_ASM (label_end));
  g_object_unref (label_end);
}

static void
vivi_code_loop_class_init (ViviCodeLoopClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_loop_dispose;

  token_class->print = vivi_code_loop_print;
  token_class->compile = vivi_code_loop_compile;
}

static void
vivi_code_loop_init (ViviCodeLoop *token)
{
}

ViviCodeStatement *
vivi_code_loop_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_LOOP, NULL);
}

void
vivi_code_loop_set_condition (ViviCodeLoop *loop, ViviCodeValue *condition,
    gboolean post_condition)
{
  g_return_if_fail (VIVI_IS_CODE_LOOP (loop));
  g_return_if_fail (VIVI_IS_CODE_VALUE (condition));

  if (condition)
    g_object_ref (condition);
  if (loop->condition)
    g_object_unref (loop->condition);
  loop->condition = condition;
  loop->post_condition = post_condition;
}

void
vivi_code_loop_set_statement (ViviCodeLoop *loop, ViviCodeStatement *statement)
{
  g_return_if_fail (VIVI_IS_CODE_LOOP (loop));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (statement));

  if (statement)
    g_object_ref (statement);
  if (loop->statement)
    g_object_unref (loop->statement);
  loop->statement = statement;
}

