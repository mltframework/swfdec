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

#ifndef _VIVI_CODE_VARIABLE_H_
#define _VIVI_CODE_VARIABLE_H_

#include <vivified/code/vivi_code_statement.h>
#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviCodeVariable ViviCodeVariable;
typedef struct _ViviCodeVariableClass ViviCodeVariableClass;

#define VIVI_TYPE_CODE_VARIABLE                    (vivi_code_variable_get_type())
#define VIVI_IS_CODE_VARIABLE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_VARIABLE))
#define VIVI_IS_CODE_VARIABLE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_VARIABLE))
#define VIVI_CODE_VARIABLE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_VARIABLE, ViviCodeVariable))
#define VIVI_CODE_VARIABLE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_VARIABLE, ViviCodeVariableClass))
#define VIVI_CODE_VARIABLE_VARIABLE_CLASS(obj)          (G_TYPE_INSTANCE_VARIABLE_CLASS ((obj), VIVI_TYPE_CODE_VARIABLE, ViviCodeVariableClass))

struct _ViviCodeVariable
{
  ViviCodeStatement	statement;

  ViviCodeValue *	name;
  ViviCodeValue	*	value;
};

struct _ViviCodeVariableClass
{
  ViviCodeStatementClass	value_class;
};

GType			vivi_code_variable_get_type   	(void);

ViviCodeStatement *	vivi_code_variable_new		(ViviCodeValue *	name,
							 ViviCodeValue *	value);
ViviCodeStatement *	vivi_code_variable_new_name	(const char *		name,
							 ViviCodeValue *	value);


G_END_DECLS
#endif
