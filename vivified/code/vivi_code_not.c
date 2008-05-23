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

#include "vivi_code_not.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeNot, vivi_code_not, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_not_dispose (GObject *object)
{
  ViviCodeNot *not = VIVI_CODE_NOT (object);

  g_object_unref (not->value);

  G_OBJECT_CLASS (vivi_code_not_parent_class)->dispose (object);
}

static ViviCodeValue * 
vivi_code_not_optimize (ViviCodeValue *value, SwfdecAsValueType hint)
{
  ViviCodeNot *not = VIVI_CODE_NOT (value);

  if (hint == SWFDEC_AS_TYPE_BOOLEAN && VIVI_IS_CODE_NOT (not->value)) {
    return vivi_code_value_optimize (VIVI_CODE_NOT (not->value)->value, hint);
  }

  /* FIXME: optimize not->value */
  return g_object_ref (not);
}

static void
vivi_code_not_print_value (ViviCodeValue *value, ViviCodePrinter *printer)
{
  ViviCodeNot *not = VIVI_CODE_NOT (value);

  vivi_code_printer_print (printer, "!");
  vivi_code_printer_print_value (printer, not->value, VIVI_PRECEDENCE_UNARY);
}

static void
vivi_code_not_compile_value (ViviCodeValue *value, ViviCodeCompiler *compiler)
{
  ViviCodeNot *not = VIVI_CODE_NOT (value);

  vivi_code_compiler_compile_value (compiler, not->value);
  vivi_code_compiler_take_code (compiler, vivi_code_asm_not_new ());
}

static gboolean
vivi_code_not_is_constant (ViviCodeValue *value)
{
  return vivi_code_value_is_constant (VIVI_CODE_NOT (value)->value);
}

static void
vivi_code_not_class_init (ViviCodeNotClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_not_dispose;

  value_class->print_value = vivi_code_not_print_value;
  value_class->compile_value = vivi_code_not_compile_value;
  value_class->is_constant = vivi_code_not_is_constant;
  value_class->optimize = vivi_code_not_optimize;
}

static void
vivi_code_not_init (ViviCodeNot *not)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (not);

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_UNARY);
}

ViviCodeValue *
vivi_code_not_new (ViviCodeValue *value)
{
  ViviCodeNot *not;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  not = g_object_new (VIVI_TYPE_CODE_NOT, NULL);
  not->value = g_object_ref (value);

  return VIVI_CODE_VALUE (not);
}

ViviCodeValue *
vivi_code_not_get_value (ViviCodeNot *not)
{
  g_return_val_if_fail (VIVI_IS_CODE_NOT (not), NULL);

  return not->value;
}

