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

#include "vivi_code_return.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCodeReturn, vivi_code_return, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_return_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  vivi_code_printer_print (printer, "return;");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_return_class_init (ViviCodeReturnClass *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  token_class->print = vivi_code_return_print;
}

static void
vivi_code_return_init (ViviCodeReturn *token)
{
}

ViviCodeToken *
vivi_code_return_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_RETURN, NULL);
}

