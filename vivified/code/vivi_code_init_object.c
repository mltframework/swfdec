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

#include "vivi_code_init_object.h"
#include "vivi_code_printer.h"
#include "vivi_code_number.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeInitObject, vivi_code_init_object, VIVI_TYPE_CODE_VALUE)

typedef struct _VariableEntry VariableEntry;
struct _VariableEntry {
  ViviCodeValue *	name;
  ViviCodeValue *	value;
};

static void
vivi_code_init_object_dispose (GObject *object)
{
  ViviCodeInitObject *obj = VIVI_CODE_INIT_OBJECT (object);
  guint i;

  for (i = 0; i < obj->variables->len; i++) {
    VariableEntry *entry = &g_array_index (obj->variables, VariableEntry, i);
    g_object_unref (entry->name);
    g_object_unref (entry->value);
  }
  g_array_free (obj->variables, TRUE);

  G_OBJECT_CLASS (vivi_code_init_object_parent_class)->dispose (object);
}

static ViviCodeValue * 
vivi_code_init_object_optimize (ViviCodeValue *value, SwfdecAsValueType hint)
{
  /* FIXME: write */

  return g_object_ref (value);
}

static void
vivi_code_init_object_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeInitObject *object = VIVI_CODE_INIT_OBJECT (token);
  guint i;

  vivi_code_printer_print (printer, "{");
  for (i = 0; i < object->variables->len; i++) {
    VariableEntry *entry = &g_array_index (object->variables, VariableEntry, i);
    if (i > 0)
      vivi_code_printer_print (printer, ", ");
    /* FIXME: precedences? */
    vivi_code_printer_print_value (printer, entry->name, VIVI_PRECEDENCE_COMMA);
    vivi_code_printer_print (printer, ": ");
    vivi_code_printer_print_value (printer, entry->value, VIVI_PRECEDENCE_COMMA);
  }
  vivi_code_printer_print (printer, "}");
}

static void
vivi_code_init_object_compile (ViviCodeToken *token,
    ViviCodeCompiler *compiler)
{
  ViviCodeInitObject *object = VIVI_CODE_INIT_OBJECT (token);
  ViviCodeValue *count;
  ViviCodeAsm *code;
  guint i;

  for (i = 0; i < object->variables->len; i++) {
    VariableEntry *entry = &g_array_index (object->variables, VariableEntry, i);
    vivi_code_compiler_compile_value (compiler, entry->value);
    vivi_code_compiler_compile_value (compiler, entry->name);
  }

  count = vivi_code_number_new (object->variables->len);
  vivi_code_compiler_compile_value (compiler, count);
  g_object_unref (count);

  code = vivi_code_asm_init_object_new ();
  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);
}

static gboolean
vivi_code_init_object_is_constant (ViviCodeValue *value)
{
  /* not constant, because we return a new object every time */
  return FALSE;
}

static void
vivi_code_init_object_class_init (ViviCodeInitObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_init_object_dispose;

  token_class->print = vivi_code_init_object_print;
  token_class->compile = vivi_code_init_object_compile;

  value_class->is_constant = vivi_code_init_object_is_constant;
  value_class->optimize = vivi_code_init_object_optimize;
}

static void
vivi_code_init_object_init (ViviCodeInitObject *object)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (object);

  object->variables = g_array_new (FALSE, FALSE, sizeof (VariableEntry));

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_PARENTHESIS);
}

ViviCodeValue *
vivi_code_init_object_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_INIT_OBJECT, NULL);
}

void
vivi_code_init_object_add_variable (ViviCodeInitObject *object,
    ViviCodeValue *name, ViviCodeValue *value)
{
  VariableEntry entry;

  g_return_if_fail (VIVI_IS_CODE_INIT_OBJECT (object));
  g_return_if_fail (VIVI_IS_CODE_VALUE (name));
  g_return_if_fail (VIVI_IS_CODE_VALUE (value));

  entry.name = g_object_ref (name);
  entry.value = g_object_ref (value);

  g_array_append_val (object->variables, entry);
}

