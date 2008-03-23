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

#include "vivi_code_value.h"

G_DEFINE_ABSTRACT_TYPE (ViviCodeValue, vivi_code_value, VIVI_TYPE_CODE_TOKEN)

static void
vivi_code_value_dispose (GObject *object)
{
  //ViviCodeValue *dec = VIVI_CODE_VAULE (object);

  G_OBJECT_CLASS (vivi_code_value_parent_class)->dispose (object);
}

static void
vivi_code_value_class_init (ViviCodeValueClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_code_value_dispose;
}

static void
vivi_code_value_init (ViviCodeValue *value)
{
  value->precedence = VIVI_PRECEDENCE_MIN;
}

gboolean
vivi_code_value_is_constant (ViviCodeValue *value)
{
  ViviCodeValueClass *klass;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), FALSE);

  klass = VIVI_CODE_VALUE_GET_CLASS (value);
  g_return_val_if_fail (klass->is_constant != NULL, FALSE);

  return klass->is_constant (value);
}

void
vivi_code_value_set_precedence (ViviCodeValue *value, ViviPrecedence precedence)
{
  g_return_if_fail (VIVI_IS_CODE_VALUE (value));

  value->precedence = precedence;
}

ViviPrecedence
vivi_code_value_get_precedence (ViviCodeValue *value)
{
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), VIVI_PRECEDENCE_MIN);

  return value->precedence;
}

ViviCodeValue *
vivi_code_value_optimize (ViviCodeValue *value, SwfdecAsValueType hint)
{
  ViviCodeValueClass *klass;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  klass = VIVI_CODE_VALUE_GET_CLASS (value);
  if (klass->optimize) {
    ViviCodeValue *ret = klass->optimize (value, hint);
    return ret;
  } else {
    return g_object_ref (value);
  }
}

gboolean
vivi_code_value_is_equal (ViviCodeValue *a, ViviCodeValue *b)
{
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (a), FALSE);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (b), FALSE);

  return a == b;
}

