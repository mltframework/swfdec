/* Vivified
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_code_throw.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCodeThrow, vivi_code_throw, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_throw_dispose (GObject *object)
{
  ViviCodeThrow *throw_ = VIVI_CODE_THROW (object);

  if (throw_->value) {
    g_object_unref (throw_->value);
    throw_->value = NULL;
  }

  G_OBJECT_CLASS (vivi_code_throw_parent_class)->dispose (object);
}

static void
vivi_code_throw_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeThrow *throw_ = VIVI_CODE_THROW (token);

  vivi_code_printer_print (printer, "return");
  if (throw_->value) {
    vivi_code_printer_print (printer, " ");
    vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (throw_->value));
  }
  vivi_code_printer_print (printer, ";");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_throw_class_init (ViviCodeThrowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_throw_dispose;

  token_class->print = vivi_code_throw_print;
}

static void
vivi_code_throw_init (ViviCodeThrow *token)
{
}

ViviCodeStatement *
vivi_code_throw_new (ViviCodeValue *value)
{
  ViviCodeThrow *throw_ = g_object_new (VIVI_TYPE_CODE_THROW, NULL);

  throw_->value = g_object_ref (value);

  return VIVI_CODE_STATEMENT (throw_);
}

ViviCodeValue *
vivi_code_throw_get_value (ViviCodeThrow *throw_)
{
  g_return_val_if_fail (VIVI_IS_CODE_THROW (throw_), NULL);

  return throw_->value;
}
