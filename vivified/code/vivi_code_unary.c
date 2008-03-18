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

#include "vivi_code_unary.h"

G_DEFINE_TYPE (ViviCodeUnary, vivi_code_unary, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_unary_dispose (GObject *object)
{
  ViviCodeUnary *unary = VIVI_CODE_UNARY (object);

  g_object_unref (unary->value);

  G_OBJECT_CLASS (vivi_code_unary_parent_class)->dispose (object);
}

static char *
vivi_code_unary_to_code (ViviCodeToken *token)
{
  ViviCodeUnary *unary = VIVI_CODE_UNARY (token);
  char *s, *ret;

  s = vivi_code_token_to_code (VIVI_CODE_TOKEN (unary->value));
  if (vivi_code_value_get_precedence (unary->value) < VIVI_PRECEDENCE_UNARY)
    ret = g_strdup_printf ("%c(%s)", unary->operation, s);
  else
    ret = g_strdup_printf ("%c%s", unary->operation, s);
  g_free (s);

  return ret;
}

static gboolean
vivi_code_unary_is_constant (ViviCodeValue *value)
{
  return vivi_code_value_is_constant (VIVI_CODE_UNARY (value)->value);
}

static void
vivi_code_unary_class_init (ViviCodeUnaryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_unary_dispose;

  token_class->to_code = vivi_code_unary_to_code;

  value_class->is_constant = vivi_code_unary_is_constant;
}

static void
vivi_code_unary_init (ViviCodeUnary *unary)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (unary);

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_UNARY);
}

ViviCodeToken *
vivi_code_unary_new (ViviCodeValue *value, char operation)
{
  ViviCodeUnary *unary;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  unary = g_object_new (VIVI_TYPE_CODE_UNARY, NULL);
  unary->value = value;
  unary->operation = operation;

  return VIVI_CODE_TOKEN (unary);
}

