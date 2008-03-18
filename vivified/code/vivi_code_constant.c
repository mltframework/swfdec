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

#include "vivi_code_constant.h"

G_DEFINE_TYPE (ViviCodeConstant, vivi_code_constant, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_constant_dispose (GObject *object)
{
  ViviCodeConstant *constant = VIVI_CODE_CONSTANT (object);

  g_free (constant->text);

  G_OBJECT_CLASS (vivi_code_constant_parent_class)->dispose (object);
}

static char *
vivi_code_constant_to_code (ViviCodeToken *token)
{
  ViviCodeConstant *constant = VIVI_CODE_CONSTANT (token);

  return g_strdup (constant->text);
}

static gboolean
vivi_code_constant_is_constant (ViviCodeValue *value)
{
  return TRUE;
}

static void
vivi_code_constant_class_init (ViviCodeConstantClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_constant_dispose;

  token_class->to_code = vivi_code_constant_to_code;

  value_class->is_constant = vivi_code_constant_is_constant;
}

static void
vivi_code_constant_init (ViviCodeConstant *constant)
{
  vivi_code_value_set_precedence (VIVI_CODE_VALUE (constant), VIVI_PRECEDENCE_MAX);
}

ViviCodeToken *
vivi_code_constant_new (const char *text)
{
  ViviCodeConstant *constant;

  g_return_val_if_fail (text != NULL, NULL);

  constant = g_object_new (VIVI_TYPE_CODE_CONSTANT, NULL);
  constant->text = g_strdup (text);

  return VIVI_CODE_TOKEN (constant);
}

ViviCodeToken *
vivi_code_constant_new_undefined (void)
{
  return vivi_code_constant_new ("undefined");
}

