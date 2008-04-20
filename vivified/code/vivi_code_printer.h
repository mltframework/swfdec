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

#ifndef _VIVI_CODE_PRINTER_H_
#define _VIVI_CODE_PRINTER_H_

#include <swfdec/swfdec.h>
#include <vivified/code/vivi_code_token.h>
#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviCodePrinterClass ViviCodePrinterClass;

#define VIVI_TYPE_CODE_PRINTER                    (vivi_code_printer_get_type())
#define VIVI_IS_CODE_PRINTER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_PRINTER))
#define VIVI_IS_CODE_PRINTER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_PRINTER))
#define VIVI_CODE_PRINTER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_PRINTER, ViviCodePrinter))
#define VIVI_CODE_PRINTER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_PRINTER, ViviCodePrinterClass))
#define VIVI_CODE_PRINTER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_PRINTER, ViviCodePrinterClass))

struct _ViviCodePrinter
{
  GObject		object;

  guint			indentation;	/* amount of indetation */
};

struct _ViviCodePrinterClass
{
  GObjectClass		object_class;

  void			(* push_token)			(ViviCodePrinter *	printer,
							 ViviCodeToken *	token);
  void			(* pop_token)			(ViviCodePrinter *	printer,
							 ViviCodeToken *	token);
  void			(* print)			(ViviCodePrinter *	printer,
							 const char *		text);
  void			(* new_line)			(ViviCodePrinter *	printer,
							 gboolean		ignore_indentation);
  void			(* push_indentation)		(ViviCodePrinter *	printer);
  void			(* pop_indentation)		(ViviCodePrinter *	printer);
};

GType			vivi_code_printer_get_type   	(void);

void			vivi_code_printer_print_token	(ViviCodePrinter *	printer,
							 ViviCodeToken *	token);
void			vivi_code_printer_print_value	(ViviCodePrinter *	printer,
							 ViviCodeValue *	value,
							 ViviPrecedence		precedence);

void			vivi_code_printer_print		(ViviCodePrinter *	printer,
							 const char *		text);
void			vivi_code_printer_new_line	(ViviCodePrinter *	printer,
							 gboolean		ignore_indentation);
void			vivi_code_printer_push_indentation
							(ViviCodePrinter *	printer);
void			vivi_code_printer_pop_indentation
							(ViviCodePrinter *	printer);
guint			vivi_code_printer_get_indentation
							(ViviCodePrinter *	printer);


char *			vivi_code_escape_string		(const char *		s);


G_END_DECLS
#endif
