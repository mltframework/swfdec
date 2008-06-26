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
#include "vivi_code_asm_pool.h"
#include "vivi_code_asm_push.h"
#include "vivi_code_comment.h"
#include "vivi_code_compiler.h"
#include "vivi_code_emitter.h"
#include "vivi_code_label.h"
#include "vivi_code_printer.h"
#include "vivi_code_undefined.h"

G_DEFINE_TYPE (ViviCodeAssembler, vivi_code_assembler, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_assembler_dispose (GObject *object)
{
  ViviCodeAssembler *assembler = VIVI_CODE_ASSEMBLER (object);

  g_ptr_array_foreach (assembler->codes, (GFunc) g_object_unref, NULL);
  g_ptr_array_free (assembler->codes, TRUE);
  g_ptr_array_foreach (assembler->arguments, (GFunc) g_object_unref, NULL);
  g_ptr_array_free (assembler->arguments, TRUE);

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

  vivi_code_printer_print (printer, "asm");
  if (assembler->arguments->len) {
    vivi_code_printer_print (printer, " (");
    for (i = 0; i < assembler->codes->len; i++) {
      if (i > 0)
	vivi_code_printer_print (printer, ", ");
      vivi_code_printer_print_value (printer, 
	  g_ptr_array_index (assembler->arguments, i), VIVI_PRECEDENCE_MEMBER);
    }
    vivi_code_printer_print (printer, ")");
  }
  vivi_code_printer_print (printer, " {");
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

  for (i = 0; i < assembler->arguments->len; i++) {
    vivi_code_compiler_compile_value (compiler, 
	g_ptr_array_index (assembler->arguments, i));
  }

  // TODO: clone?
  for (i = 0; i < assembler->codes->len; i++) {
    vivi_code_compiler_add_code (compiler,
	g_ptr_array_index (assembler->codes, i));
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
  assembler->arguments = g_ptr_array_new ();
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

/*** ARGUMENT HANDLING ***/

guint
vivi_code_assembler_get_n_arguments (ViviCodeAssembler *assembler)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASSEMBLER (assembler), 0);

  return assembler->arguments->len;
}

void
vivi_code_assembler_push_argument (ViviCodeAssembler *assembler, ViviCodeValue *value)
{
  g_return_if_fail (VIVI_IS_CODE_ASSEMBLER (assembler));
  g_return_if_fail (VIVI_IS_CODE_VALUE (value));

  g_object_ref (value);
  g_ptr_array_add (assembler->arguments, value);
}

ViviCodeValue *
vivi_code_assembler_pop_argument (ViviCodeAssembler *assembler)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASSEMBLER (assembler), NULL);

  if (assembler->arguments->len == 0) {
    return vivi_code_undefined_new ();
  } else {
    return g_ptr_array_remove_index (assembler->arguments, 
	assembler->arguments->len - 1);
  }
}

ViviCodeValue *
vivi_code_assembler_get_argument (ViviCodeAssembler *assembler, guint i)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASSEMBLER (assembler), NULL);
  g_return_val_if_fail (i < assembler->arguments->len, NULL);

  return g_ptr_array_index (assembler->arguments, i);
}

/*** OPTIMIZATION ***/

gboolean
vivi_code_assembler_pool (ViviCodeAssembler *assembler)
{
  guint i, j, num;
  gsize length;
  GSList *list, *iter;
  SwfdecBots *bots;
  SwfdecBuffer *buffer;
  SwfdecConstantPool *pool;
  ViviCodeAsm *code;

  // collect the strings
  list = NULL;
  length = 0;
  num = 0;
  for (i = 0; i < assembler->codes->len; i++) {
    // if there is already a pool, don't touch anything
    if (VIVI_IS_CODE_ASM_POOL (g_ptr_array_index (assembler->codes, i))) {
      g_slist_foreach (list, (GFunc)g_free, NULL);
      g_slist_free (list);
      return FALSE;
    }

    if (VIVI_IS_CODE_ASM_PUSH (g_ptr_array_index (assembler->codes, i))) {
      ViviCodeAsmPush *push =
	VIVI_CODE_ASM_PUSH (g_ptr_array_index (assembler->codes, i));
      for (j = 0; j < vivi_code_asm_push_get_n_values (push); j++) {
	ViviCodeConstantType type =
	  vivi_code_asm_push_get_value_type (push, j);

	// if there already is something that tries to access a pool,
	// don't touch anything
	if (type == VIVI_CODE_CONSTANT_CONSTANT_POOL ||
	    type == VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG) {
	  g_slist_foreach (list, (GFunc)g_free, NULL);
	  g_slist_free (list);
	  return FALSE;
	}

	if (type == VIVI_CODE_CONSTANT_STRING) {
	  const char *str = vivi_code_asm_push_get_string (push, j);
	  if (!g_slist_find_custom (list, str, (GCompareFunc)strcmp) &&
	      length + strlen (str) + 1 < G_MAXUINT16) {
	    list = g_slist_prepend (list, g_strdup (str));
	    length += strlen (str) + 1;
	    num++;
	  }
	}
      }
    }
  }

  if (num == 0) {
    g_slist_foreach (list, (GFunc)g_free, NULL);
    g_slist_free (list);
    return TRUE;
  }

  // so we have first encountered strings first
  list = g_slist_reverse (list);

  // create the pool action
  bots = swfdec_bots_open ();
  swfdec_bots_put_u16 (bots, num);
  for (iter = list; iter != NULL; iter = iter->next) {
    swfdec_bots_put_string (bots, iter->data);
  }
  buffer = swfdec_bots_close (bots);

  // FIXME: version
  pool = swfdec_constant_pool_new (NULL, buffer, 8);
  g_assert (pool != NULL);
  code = vivi_code_asm_pool_new (pool);
  g_assert (code != NULL);
  swfdec_constant_pool_unref (pool);
  swfdec_buffer_unref (buffer);

  vivi_code_assembler_insert_code (assembler, 0, code);
  g_object_unref (code);

  // change the pushes
  for (i = 0; i < assembler->codes->len; i++) {
    if (VIVI_IS_CODE_ASM_PUSH (g_ptr_array_index (assembler->codes, i))) {
      ViviCodeAsmPush *push =
	VIVI_CODE_ASM_PUSH (g_ptr_array_index (assembler->codes, i));

      for (j = 0; j < vivi_code_asm_push_get_n_values (push); j++) {
	const char *string;
	guint k;

	if (vivi_code_asm_push_get_value_type (push, j) !=
	    VIVI_CODE_CONSTANT_STRING)
	  continue;

	string = vivi_code_asm_push_get_string (push, j);
	k = 0;
	for (iter = list; iter != NULL; iter = iter->next) {
	  if (strcmp (iter->data, string) == 0)
	    break;
	  k++;
	}

	if (iter != NULL) {
	  vivi_code_asm_push_remove_value (push, j);
	  vivi_code_asm_push_insert_pool (push, j, k);
	}
      }
    }
  }

  // done
  g_slist_foreach (list, (GFunc)g_free, NULL);
  g_slist_free (list);

  return TRUE;
}

static void
vivi_code_assembler_get_stack_change (ViviCodeAsm *code, int *add, int *remove)
{
  ViviCodeAsmInterface *iface;
  int add_, remove_;

  iface = VIVI_CODE_ASM_GET_INTERFACE (code);
  g_assert (iface->get_stack_change != NULL);
  iface->get_stack_change (code, &add_, &remove_);

  if (add != NULL)
    *add = add_;
  if (remove != NULL)
    *remove = remove_;
}

void
vivi_code_assembler_merge_push (ViviCodeAssembler *assembler, guint max_depth)
{
  ViviCodeAsmPush *previous;
  guint i, j, depth;
  int add, remove;

  depth = 0;
  previous = NULL;
  for (i = 0; i < assembler->codes->len; i++) {
    ViviCodeAsm *code =
      VIVI_CODE_ASM (g_ptr_array_index (assembler->codes, i));

    if (VIVI_IS_CODE_ASM_PUSH (code)) {
      ViviCodeAsmPush *current = VIVI_CODE_ASM_PUSH (code);

      if (previous != NULL &&
	  vivi_code_asm_push_get_n_values (previous) >= depth) {
	for (j = 0; j < vivi_code_asm_push_get_n_values (current); j++) {
	  vivi_code_asm_push_copy_value (previous,
	      vivi_code_asm_push_get_n_values (previous) - depth, current, j);
	}
	vivi_code_assembler_remove_code (assembler, VIVI_CODE_ASM (current));
	i--;
      } else {
	previous = current;
	depth = 0;
      }
    } else {
      if (previous != NULL) {
	if (VIVI_IS_CODE_LABEL (code)) {
	  previous = NULL;
	  depth = 0;
	} else {
	  vivi_code_assembler_get_stack_change (code, &add, &remove);
	  if (add != 0) {
	    previous = NULL;
	    depth = 0;
	  } else {
	    if (remove == -1) {
	      previous = NULL;
	    } else {
	      depth += remove;
	      if (depth > max_depth) {
		previous = NULL;
		depth = 0;
	      }
	    }
	  }
	}
      }
    }
  }
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
  /* FIXME: should this work? */
  g_return_val_if_fail (assembler->arguments->len == 0, NULL);

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

