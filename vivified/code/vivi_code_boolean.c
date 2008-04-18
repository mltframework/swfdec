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

#include "vivi_code_boolean.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCodeBoolean, vivi_code_boolean, VIVI_TYPE_CODE_CONSTANT)

static void
vivi_code_boolean_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeBoolean *b = VIVI_CODE_BOOLEAN (token);

  vivi_code_printer_print (printer, b->value ? "true" : "false");
}

static char *
vivi_code_boolean_get_variable_name (ViviCodeConstant *constant)
{
  ViviCodeBoolean *b = VIVI_CODE_BOOLEAN (constant);

  return g_strdup (b->value ? "true" : "false");
}

static void
vivi_code_boolean_class_init (ViviCodeBooleanClass *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeConstantClass *constant_class = VIVI_CODE_CONSTANT_CLASS (klass);

  token_class->print = vivi_code_boolean_print;

  constant_class->get_variable_name = vivi_code_boolean_get_variable_name;
}

static void
vivi_code_boolean_init (ViviCodeBoolean *boolean)
{
}

ViviCodeValue *
vivi_code_boolean_new (gboolean value)
{
  ViviCodeBoolean *b;

  b = g_object_new (VIVI_TYPE_CODE_BOOLEAN, NULL);
  b->value = value;

  return VIVI_CODE_VALUE (b);
}

gboolean
vivi_code_boolean_get_value (ViviCodeBoolean *b)
{
  g_return_val_if_fail (VIVI_IS_CODE_BOOLEAN (b), FALSE);

  return b->value;
}

