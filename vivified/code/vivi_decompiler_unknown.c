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

#include "vivi_decompiler_unknown.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviDecompilerUnknown, vivi_decompiler_unknown, VIVI_TYPE_CODE_VALUE)

static void
vivi_decompiler_unknown_dispose (GObject *object)
{
  ViviDecompilerUnknown *unknown = VIVI_DECOMPILER_UNKNOWN (object);

  g_free (unknown->name);
  if (unknown->value)
    g_object_unref (unknown->value);

  G_OBJECT_CLASS (vivi_decompiler_unknown_parent_class)->dispose (object);
}

static void
vivi_decompiler_unknown_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviDecompilerUnknown *unknown = VIVI_DECOMPILER_UNKNOWN (token);

  if (unknown->value) {
    vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (unknown->value));
  } else {
    g_printerr ("FIXME: printing unknown!\n");
    vivi_code_printer_print (printer, unknown->name);
  }
}

static gboolean
vivi_decompiler_unknown_is_constant (ViviCodeValue *value)
{
  ViviDecompilerUnknown *unknown = VIVI_DECOMPILER_UNKNOWN (value);
  
  if (unknown->value)
    return vivi_code_value_is_constant (unknown->value);
  else
    return FALSE;
}

static ViviCodeValue *
vivi_decompiler_unknown_optimize (ViviCodeValue *value, SwfdecAsValueType hint)
{
  ViviDecompilerUnknown *unknown = VIVI_DECOMPILER_UNKNOWN (value);
  
  if (unknown->value)
    return vivi_code_value_optimize (unknown->value, hint);
  else
    return g_object_ref (unknown);
}

static void
vivi_decompiler_unknown_class_init (ViviDecompilerUnknownClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_decompiler_unknown_dispose;

  token_class->print = vivi_decompiler_unknown_print;

  value_class->is_constant = vivi_decompiler_unknown_is_constant;
  value_class->optimize = vivi_decompiler_unknown_optimize;
}

static void
vivi_decompiler_unknown_init (ViviDecompilerUnknown *unknown)
{
  vivi_code_value_set_precedence (VIVI_CODE_VALUE (unknown), VIVI_PRECEDENCE_MEMBER);
}

ViviCodeValue *
vivi_decompiler_unknown_new (const char *name)
{
  ViviDecompilerUnknown *unknown;

  g_return_val_if_fail (name != NULL, NULL);

  unknown = g_object_new (VIVI_TYPE_DECOMPILER_UNKNOWN, NULL);
  unknown->name = g_strdup (name);

  return VIVI_CODE_VALUE (unknown);
}

void
vivi_decompiler_unknown_set_value (ViviDecompilerUnknown *unknown, ViviCodeValue *value)
{
  g_return_if_fail (VIVI_IS_DECOMPILER_UNKNOWN (unknown));
  g_return_if_fail (VIVI_IS_CODE_VALUE (value));

  g_object_ref (value);
  if (unknown->value)
    g_object_unref (unknown->value);
  unknown->value = value;

  vivi_code_value_set_precedence (VIVI_CODE_VALUE (unknown),
      vivi_code_value_get_precedence (value));
}

ViviCodeValue *
vivi_decompiler_unknown_get_value (ViviDecompilerUnknown *unknown)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER_UNKNOWN (unknown), NULL);

  return unknown->value;
}

const char *
vivi_decompiler_unknown_get_name (ViviDecompilerUnknown *unknown)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER_UNKNOWN (unknown), NULL);

  return unknown->name;
}

