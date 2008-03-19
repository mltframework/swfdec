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

#include "vivi_code_text_printer.h"

G_DEFINE_TYPE (ViviCodeTextPrinter, vivi_code_text_printer, VIVI_TYPE_CODE_PRINTER)

static void
vivi_code_text_printer_dispose (GObject *object)
{
  //ViviCodeTextPrinter *dec = VIVI_CODE_VAULE (object);

  G_OBJECT_CLASS (vivi_code_text_printer_parent_class)->dispose (object);
}

static void
vivi_code_text_printer_print (ViviCodePrinter *printer, const char *text)
{
  ViviCodeTextPrinter *tp = VIVI_CODE_TEXT_PRINTER (printer);

  if (tp->need_indent) {
    g_print ("%*s", vivi_code_printer_get_indentation (printer) * 2, "");
    tp->need_indent = FALSE;
  }
  g_print ("%s", text);
}

static void
vivi_code_text_printer_new_line (ViviCodePrinter *printer, gboolean ignore_indentation)
{
  ViviCodeTextPrinter *text = VIVI_CODE_TEXT_PRINTER (printer);

  g_print ("\n");
  text->need_indent = !ignore_indentation;
}

static void
vivi_code_text_printer_class_init (ViviCodeTextPrinterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodePrinterClass *printer_class = VIVI_CODE_PRINTER_CLASS (klass);

  object_class->dispose = vivi_code_text_printer_dispose;

  printer_class->print = vivi_code_text_printer_print;
  printer_class->new_line = vivi_code_text_printer_new_line;
}

static void
vivi_code_text_printer_init (ViviCodeTextPrinter *tp)
{
  tp->need_indent = TRUE;
}

ViviCodePrinter *
vivi_code_text_printer_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_TEXT_PRINTER, NULL);
}

