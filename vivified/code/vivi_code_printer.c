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

#include "vivi_code_printer.h"

G_DEFINE_ABSTRACT_TYPE (ViviCodePrinter, vivi_code_printer, G_TYPE_OBJECT)

static void
vivi_code_printer_dispose (GObject *object)
{
  //ViviCodePrinter *dec = VIVI_CODE_VAULE (object);

  G_OBJECT_CLASS (vivi_code_printer_parent_class)->dispose (object);
}

static void
vivi_code_printer_class_init (ViviCodePrinterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_code_printer_dispose;
}

static void
vivi_code_printer_init (ViviCodePrinter *printer)
{
}

void
vivi_code_printer_print_token (ViviCodePrinter *printer, ViviCodeToken *token)
{
  ViviCodePrinterClass *klass;
  ViviCodeTokenClass *token_class;

  g_return_if_fail (VIVI_IS_CODE_PRINTER (printer));
  g_return_if_fail (VIVI_IS_CODE_TOKEN (token));

  klass = VIVI_CODE_PRINTER_GET_CLASS (printer);
  if (klass->push_token)
    klass->push_token (printer, token);
  token_class = VIVI_CODE_TOKEN_GET_CLASS (token);
  g_return_if_fail (token_class->print);
  token_class->print (token, printer);
  if (klass->pop_token)
    klass->pop_token (printer, token);
}

void
vivi_code_printer_print (ViviCodePrinter *printer, const char *text)
{
  ViviCodePrinterClass *klass;

  g_return_if_fail (VIVI_IS_CODE_PRINTER (printer));
  g_return_if_fail (text != NULL);

  klass = VIVI_CODE_PRINTER_GET_CLASS (printer);
  g_return_if_fail (klass->print);
  klass->print (printer, text);
}

void
vivi_code_printer_print_value (ViviCodePrinter *printer, ViviCodeValue *value,
    ViviPrecedence precedence)
{
  gboolean needs_parens;

  g_return_if_fail (VIVI_IS_CODE_PRINTER (printer));
  g_return_if_fail (VIVI_IS_CODE_VALUE (value));

  needs_parens = vivi_code_value_get_precedence (value) < precedence;
  if (needs_parens)
    vivi_code_printer_print (printer, "(");
  vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (value));
  if (needs_parens)
    vivi_code_printer_print (printer, ")");
}

void
vivi_code_printer_new_line (ViviCodePrinter *printer, gboolean ignore_indentation)
{
  ViviCodePrinterClass *klass;

  g_return_if_fail (VIVI_IS_CODE_PRINTER (printer));

  klass = VIVI_CODE_PRINTER_GET_CLASS (printer);
  if (klass->new_line)
    klass->new_line (printer, ignore_indentation);
}

void
vivi_code_printer_push_indentation (ViviCodePrinter *printer)
{
  g_return_if_fail (VIVI_IS_CODE_PRINTER (printer));

  printer->indentation++;
}

void
vivi_code_printer_pop_indentation (ViviCodePrinter *printer)
{
  g_return_if_fail (VIVI_IS_CODE_PRINTER (printer));
  g_return_if_fail (printer->indentation > 0);

  printer->indentation--;
}

guint
vivi_code_printer_get_indentation (ViviCodePrinter *printer)
{
  g_return_val_if_fail (VIVI_IS_CODE_PRINTER (printer), 0);

  return printer->indentation;
}
