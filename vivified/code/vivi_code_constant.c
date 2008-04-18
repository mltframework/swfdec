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

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "vivi_code_constant.h"
#include "vivi_code_printer.h"

G_DEFINE_ABSTRACT_TYPE (ViviCodeConstant, vivi_code_constant, VIVI_TYPE_CODE_VALUE)

static gboolean
vivi_code_constant_is_constant (ViviCodeValue *value)
{
  return TRUE;
}

static void
vivi_code_constant_class_init (ViviCodeConstantClass *klass)
{
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  value_class->is_constant = vivi_code_constant_is_constant;
}

static void
vivi_code_constant_init (ViviCodeConstant *constant)
{
  vivi_code_value_set_precedence (VIVI_CODE_VALUE (constant), VIVI_PRECEDENCE_MAX);
}

char *
vivi_code_constant_get_variable_name (ViviCodeConstant *constant)
{
  ViviCodeConstantClass *klass;

  g_return_val_if_fail (VIVI_IS_CODE_CONSTANT (constant), NULL);

  klass = VIVI_CODE_CONSTANT_GET_CLASS (constant);
  if (klass->get_variable_name == NULL)
    return NULL;

  return klass->get_variable_name (constant);
};

