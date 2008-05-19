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

#include "vivi_decompiler_duplicate.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviDecompilerDuplicate, vivi_decompiler_duplicate, VIVI_TYPE_CODE_VALUE)

static void
vivi_decompiler_duplicate_dispose (GObject *object)
{
  ViviDecompilerDuplicate *dupl = VIVI_DECOMPILER_DUPLICATE (object);

  g_object_unref (dupl->value);

  G_OBJECT_CLASS (vivi_decompiler_duplicate_parent_class)->dispose (object);
}

static void
vivi_decompiler_duplicate_print_value (ViviCodeValue *value,
    ViviCodePrinter *printer)
{
  ViviDecompilerDuplicate *dupl = VIVI_DECOMPILER_DUPLICATE (value);

  g_printerr ("FIXME: printing duplicate!\n");
  vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (dupl->value));
}

static gboolean
vivi_decompiler_duplicate_is_constant (ViviCodeValue *value)
{
  return vivi_code_value_is_constant (VIVI_DECOMPILER_DUPLICATE (value)->value);
}

static void
vivi_decompiler_duplicate_class_init (ViviDecompilerDuplicateClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_decompiler_duplicate_dispose;

  value_class->print_value = vivi_decompiler_duplicate_print_value;
  value_class->is_constant = vivi_decompiler_duplicate_is_constant;
}

static void
vivi_decompiler_duplicate_init (ViviDecompilerDuplicate *dupl)
{
}

ViviCodeValue *
vivi_decompiler_duplicate_new (ViviCodeValue *value)
{
  ViviDecompilerDuplicate *dupl;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  dupl = g_object_new (VIVI_TYPE_DECOMPILER_DUPLICATE, NULL);
  dupl->value = g_object_ref (value);
  vivi_code_value_set_precedence (VIVI_CODE_VALUE (dupl), 
      vivi_code_value_get_precedence (value));

  return VIVI_CODE_VALUE (dupl);
}

ViviCodeValue *
vivi_decompiler_duplicate_get_value (ViviDecompilerDuplicate *dupl)
{
  g_return_val_if_fail (VIVI_IS_DECOMPILER_DUPLICATE (dupl), NULL);

  return dupl->value;
}

