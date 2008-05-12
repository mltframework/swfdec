/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
 *               2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _VIVI_CODE_FUNCTION_DECLARATION_H_
#define _VIVI_CODE_FUNCTION_DECLARATION_H_

#include <vivified/code/vivi_code_statement.h>

G_BEGIN_DECLS


typedef struct _ViviCodeFunctionDeclaration ViviCodeFunctionDeclaration;
typedef struct _ViviCodeFunctionDeclarationClass ViviCodeFunctionDeclarationClass;

#define VIVI_TYPE_CODE_FUNCTION_DECLARATION                    (vivi_code_function_declaration_get_type())
#define VIVI_IS_CODE_FUNCTION_DECLARATION(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_FUNCTION_DECLARATION))
#define VIVI_IS_CODE_FUNCTION_DECLARATION_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_FUNCTION_DECLARATION))
#define VIVI_CODE_FUNCTION_DECLARATION(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_FUNCTION_DECLARATION, ViviCodeFunctionDeclaration))
#define VIVI_CODE_FUNCTION_DECLARATION_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_FUNCTION_DECLARATION, ViviCodeFunctionDeclarationClass))
#define VIVI_CODE_FUNCTION_DECLARATION_FUNCTION_DECLARATION_CLASS(obj)          (G_TYPE_INSTANCE_FUNCTION_DECLARATION_CLASS ((obj), VIVI_TYPE_CODE_FUNCTION_DECLARATION, ViviCodeFunctionDeclarationClass))

struct _ViviCodeFunctionDeclaration
{
  ViviCodeStatement	statement;

  char *			name;
  GPtrArray *			arguments;	// char *
  ViviCodeStatement *		body;
};

struct _ViviCodeFunctionDeclarationClass
{
  ViviCodeStatementClass	statement_class;
};

GType		vivi_code_function_declaration_get_type   	(void);

ViviCodeStatement *vivi_code_function_declaration_new		(const char *			name);

void		vivi_code_function_declaration_set_body	(ViviCodeFunctionDeclaration *		function,
								 ViviCodeStatement *		body);
void		vivi_code_function_declaration_add_argument	(ViviCodeFunctionDeclaration *	function,
								 const char *			name);


G_END_DECLS
#endif
