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

#include "vivi_decompiler_value.h"


#define VIVI_IS_DECOMPILER_VALUE(val) ((val) != NULL)

struct _ViviDecompilerValue {
  char *		text;
  ViviPrecedence	precedence;
  gboolean		constant;
};

void
vivi_decompiler_value_free (ViviDecompilerValue *value)
{
  g_free (value->text);
  g_slice_free (ViviDecompilerValue, value);
}

ViviDecompilerValue *
vivi_decompiler_value_new (ViviPrecedence precedence, gboolean constant, char *text)
{
  ViviDecompilerValue *value;

  g_return_val_if_fail (text != NULL, NULL);

  value = g_slice_new0 (ViviDecompilerValue);

  value->text = text;
  value->precedence = precedence;
  value->constant = constant;

  return value;
}

ViviDecompilerValue *
vivi_decompiler_value_new_printf (ViviPrecedence precedence, gboolean constant, 
    const char *format, ...)
{
  va_list varargs;
  char *s;

  g_return_val_if_fail (format != NULL, NULL);

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);

  return vivi_decompiler_value_new (precedence, constant, s);
}

ViviDecompilerValue *
vivi_decompiler_value_copy (const ViviDecompilerValue *src)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER_VALUE (src), NULL);

  return vivi_decompiler_value_new (src->precedence, src->constant, g_strdup (src->text));
}

const char * 
vivi_decompiler_value_get_text (const ViviDecompilerValue *value)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER_VALUE (value), "undefined");

  return value->text;
}

const ViviDecompilerValue *
vivi_decompiler_value_get_undefined (void)
{
  static const ViviDecompilerValue undefined = { (char *) "undefined", VIVI_PRECEDENCE_NONE, TRUE };

  return &undefined;
}

ViviPrecedence
vivi_decompiler_value_get_precedence (const ViviDecompilerValue *value)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER_VALUE (value), VIVI_PRECEDENCE_NONE);

  return value->precedence;
}

gboolean
vivi_decompiler_value_is_constant (const ViviDecompilerValue *value)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER_VALUE (value), FALSE);

  return value->constant;
}

ViviDecompilerValue *
vivi_decompiler_value_ensure_precedence (ViviDecompilerValue *value, ViviPrecedence precedence)
{
  ViviDecompilerValue *new;

  g_return_val_if_fail (VIVI_IS_DECOMPILER_VALUE (value), value);

  if (value->precedence <= precedence)
    return value;

  new = vivi_decompiler_value_new_printf (VIVI_PRECEDENCE_NONE, value->constant, 
      "(%s)", value->text);
  vivi_decompiler_value_free (value);

  return new;
}

