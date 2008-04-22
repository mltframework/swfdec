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

#include <string.h>
#include <swfdec/swfdec_script_internal.h>

#include "vivi_code_assembler.h"
#include "vivi_code_comment.h"
#include "vivi_code_emitter.h"
#include "vivi_code_label.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"

G_DEFINE_TYPE (ViviCodeAssembler, vivi_code_assembler, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_assembler_dispose (GObject *object)
{
  ViviCodeAssembler *assembler = VIVI_CODE_ASSEMBLER (object);
  guint i;

  for (i = 0; i < assembler->codes->len; i++) {
    g_object_unref (g_ptr_array_index (assembler->codes, i));
  }
  g_ptr_array_free (assembler->codes, TRUE);

  G_OBJECT_CLASS (vivi_code_assembler_parent_class)->dispose (object);
}

static ViviCodeStatement *
vivi_code_assembler_optimize (ViviCodeStatement *stmt)
{
  return g_object_ref (stmt);
}

static void
vivi_code_assembler_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeAssembler *assembler = VIVI_CODE_ASSEMBLER (token);
  guint i;

  vivi_code_printer_print (printer, "asm {");
  vivi_code_printer_new_line (printer, FALSE);
  vivi_code_printer_push_indentation (printer);
  for (i = 0; i < assembler->codes->len; i++) {
    vivi_code_printer_print_token (printer, g_ptr_array_index (assembler->codes, i));
  }
  vivi_code_printer_pop_indentation (printer);
  vivi_code_printer_print (printer, "}");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_assembler_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeAssembler *assembler = VIVI_CODE_ASSEMBLER (token);
  guint i;

  for (i = 0; i < assembler->codes->len; i++) {
    vivi_code_compiler_compile_token (compiler,
	VIVI_CODE_TOKEN (g_ptr_array_index (assembler->codes, i)));
  }
}

static void
vivi_code_assembler_class_init (ViviCodeAssemblerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeStatementClass *code_class = VIVI_CODE_STATEMENT_CLASS (klass);

  object_class->dispose = vivi_code_assembler_dispose;

  token_class->print = vivi_code_assembler_print;
  token_class->compile = vivi_code_assembler_compile;

  code_class->optimize = vivi_code_assembler_optimize;
}

static void
vivi_code_assembler_init (ViviCodeAssembler *assembler)
{
  assembler->codes = g_ptr_array_new ();
}

ViviCodeStatement *
vivi_code_assembler_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_ASSEMBLER, NULL);
}

guint
vivi_code_assembler_get_n_codes (ViviCodeAssembler *assembler)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASSEMBLER (assembler), 0);

  return assembler->codes->len;
}

ViviCodeAsm *
vivi_code_assembler_get_code (ViviCodeAssembler *assembler, guint i)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASSEMBLER (assembler), NULL);
  g_return_val_if_fail (i < assembler->codes->len, NULL);

  return g_ptr_array_index (assembler->codes, i);
}

void
vivi_code_assembler_insert_code (ViviCodeAssembler *assembler, guint i, ViviCodeAsm *code)
{
  g_return_if_fail (VIVI_IS_CODE_ASSEMBLER (assembler));
  g_return_if_fail (VIVI_IS_CODE_ASM (code));

  g_object_ref (code);
  if (i >= assembler->codes->len) {
    g_ptr_array_add (assembler->codes, code);
  } else {
    guint after = assembler->codes->len - i;
    g_ptr_array_set_size (assembler->codes, assembler->codes->len + 1);
    memmove (&g_ptr_array_index (assembler->codes, i + 1),
	&g_ptr_array_index (assembler->codes, i), sizeof (gpointer) * after);
    g_ptr_array_index (assembler->codes, i) = code;
  }
}

void
vivi_code_assembler_remove_code (ViviCodeAssembler *assembler, ViviCodeAsm *code)
{
  g_return_if_fail (VIVI_IS_CODE_ASSEMBLER (assembler));
  g_return_if_fail (VIVI_IS_CODE_ASM (code));

  if (g_ptr_array_remove (assembler->codes, code))
    g_object_unref (code);
  else
    g_return_if_reached ();
}

SwfdecScript *
vivi_code_assembler_assemble_script (ViviCodeAssembler *assembler,
    guint version, GError **error)
{
  ViviCodeEmitter *emit;
  SwfdecBuffer *buffer;
  SwfdecScript *script;
  guint i;

  g_return_val_if_fail (VIVI_IS_CODE_ASSEMBLER (assembler), NULL);

  emit = vivi_code_emitter_new (version);
  for (i = 0; i < assembler->codes->len; i++) {
    if (!vivi_code_emitter_emit_asm (emit, 
	  g_ptr_array_index (assembler->codes, i), error)) {
      g_object_unref (emit);
      return NULL;
    }
  }
  buffer = vivi_code_emitter_finish (emit, error);
  g_object_unref (emit);
  if (buffer == NULL)
    return NULL;
  script = swfdec_script_new (buffer, "compiled", version);
  return script;
}

