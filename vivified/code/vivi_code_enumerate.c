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

#include "vivi_code_enumerate.h"
#include "vivi_code_printer.h"
#include "vivi_code_get.h"
#include "vivi_code_string.h"
#include "vivi_code_compiler.h"
#include "vivi_code_label.h"
#include "vivi_code_asm_if.h"
#include "vivi_code_asm_jump.h"
#include "vivi_code_asm_push.h"
#include "vivi_code_asm_store.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeEnumerate, vivi_code_enumerate, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_enumerate_dispose (GObject *object)
{
  ViviCodeEnumerate *enumerate = VIVI_CODE_ENUMERATE (object);

  g_object_unref (enumerate->target);
  g_object_unref (enumerate->name);
  if (enumerate->statement)
    g_object_unref (enumerate->statement);

  G_OBJECT_CLASS (vivi_code_enumerate_parent_class)->dispose (object);
}

static void
vivi_code_enumerate_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeEnumerate *enumerate= VIVI_CODE_ENUMERATE (token);
  gboolean needs_braces;

  needs_braces = enumerate->statement &&
    vivi_code_statement_needs_braces (enumerate->statement);

  // FIXME: member

  vivi_code_printer_print (printer, "while (");
  if (enumerate->local)
    vivi_code_printer_print (printer, "var ");
  vivi_code_printer_print_value (printer, enumerate->name,
      VIVI_PRECEDENCE_MIN);
  vivi_code_printer_print (printer, " in ");
  vivi_code_printer_print_value (printer, enumerate->target,
      VIVI_PRECEDENCE_MIN);
  vivi_code_printer_print (printer, ")");

  if (needs_braces)
    vivi_code_printer_print (printer, " {");
  vivi_code_printer_new_line (printer, FALSE);
  vivi_code_printer_push_indentation (printer);

  if (enumerate->statement) {
    vivi_code_printer_print_token (printer,
	VIVI_CODE_TOKEN (enumerate->statement));
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
vivi_code_enumerate_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeEnumerate *enumerate = VIVI_CODE_ENUMERATE (token);
  ViviCodeLabel *label_start, *label_end = NULL;
  ViviCodeAsm *push;

  vivi_code_compiler_compile_value (compiler, enumerate->target);
  vivi_code_compiler_take_code (compiler, vivi_code_asm_enumerate2_new ());

  label_start = vivi_code_compiler_create_label (compiler, "enumerate_start");
  vivi_code_compiler_add_code (compiler, VIVI_CODE_ASM (label_start));

  if (enumerate->from) {
    vivi_code_compiler_take_code (compiler, vivi_code_asm_store_new (0));
  } else {
    vivi_code_compiler_take_code (compiler,
	vivi_code_asm_push_duplicate_new ());
  }
  push = vivi_code_asm_push_new ();
  vivi_code_asm_push_add_null (VIVI_CODE_ASM_PUSH (push));
  vivi_code_compiler_take_code (compiler, push);

  vivi_code_compiler_take_code (compiler, vivi_code_asm_equals2_new ());
  label_end = vivi_code_compiler_create_label (compiler, "enumerate_end");
  vivi_code_compiler_take_code (compiler, vivi_code_asm_if_new (label_end));

  if (enumerate->from) {
    vivi_code_compiler_compile_value (compiler, enumerate->from);
    vivi_code_compiler_compile_value (compiler, enumerate->name);
    push = vivi_code_asm_push_new ();
    vivi_code_asm_push_add_register (VIVI_CODE_ASM_PUSH (push), 0);
    vivi_code_compiler_take_code (compiler, push);
    g_assert (enumerate->local == FALSE);
    vivi_code_compiler_take_code (compiler, vivi_code_asm_set_member_new ());
  } else {
    vivi_code_compiler_compile_value (compiler, enumerate->name);
    vivi_code_compiler_take_code (compiler, vivi_code_asm_swap_new ());
    if (enumerate->local) {
      vivi_code_compiler_take_code (compiler,
	  vivi_code_asm_define_local_new ());
    } else {
      vivi_code_compiler_take_code (compiler,
	  vivi_code_asm_set_variable_new ());
    }
  }

  if (enumerate->statement)
    vivi_code_compiler_compile_statement (compiler, enumerate->statement);

  vivi_code_compiler_take_code (compiler,
      vivi_code_asm_jump_new (label_start));
  g_object_unref (label_start);

  vivi_code_compiler_take_code (compiler, VIVI_CODE_ASM (label_end));
  if (!enumerate->from)
    vivi_code_compiler_take_code (compiler, vivi_code_asm_pop_new ());
}

static void
vivi_code_enumerate_class_init (ViviCodeEnumerateClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_enumerate_dispose;

  token_class->print = vivi_code_enumerate_print;
  token_class->compile = vivi_code_enumerate_compile;
}

static void
vivi_code_enumerate_init (ViviCodeEnumerate *token)
{
}

ViviCodeStatement *
vivi_code_enumerate_new (ViviCodeValue *target, ViviCodeValue *from,
    ViviCodeValue *name, gboolean local)
{
  ViviCodeEnumerate *enumerate;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (target), NULL);
  g_return_val_if_fail (from == NULL || VIVI_IS_CODE_VALUE (from), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (name), NULL);
  g_return_val_if_fail (local == FALSE || from == NULL, NULL);

  enumerate = g_object_new (VIVI_TYPE_CODE_ENUMERATE, NULL);

  enumerate->target = g_object_ref (target);
  enumerate->from = g_object_ref (from);
  enumerate->name = g_object_ref (name);
  enumerate->local = local;

  return VIVI_CODE_STATEMENT (enumerate);
}

void
vivi_code_enumerate_set_statement (ViviCodeEnumerate *enumerate,
    ViviCodeStatement *statement)
{
  g_return_if_fail (VIVI_IS_CODE_ENUMERATE (enumerate));
  g_return_if_fail (VIVI_IS_CODE_STATEMENT (statement));

  if (statement)
    g_object_ref (statement);
  if (enumerate->statement)
    g_object_unref (enumerate->statement);
  enumerate->statement = statement;
}

