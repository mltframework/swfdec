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

#ifndef _VIVI_CODE_FUNCTION_H_
#define _VIVI_CODE_FUNCTION_H_

#include <vivified/code/vivi_code_value.h>
#include <vivified/code/vivi_code_statement.h>

G_BEGIN_DECLS


typedef struct _ViviCodeFunction ViviCodeFunction;
typedef struct _ViviCodeFunctionClass ViviCodeFunctionClass;

#define VIVI_TYPE_CODE_FUNCTION                    (vivi_code_function_get_type())
#define VIVI_IS_CODE_FUNCTION(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_FUNCTION))
#define VIVI_IS_CODE_FUNCTION_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_FUNCTION))
#define VIVI_CODE_FUNCTION(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_FUNCTION, ViviCodeFunction))
#define VIVI_CODE_FUNCTION_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_FUNCTION, ViviCodeFunctionClass))
#define VIVI_CODE_FUNCTION_FUNCTION_CLASS(obj)          (G_TYPE_INSTANCE_FUNCTION_CLASS ((obj), VIVI_TYPE_CODE_FUNCTION, ViviCodeFunctionClass))

struct _ViviCodeFunction
{
  ViviCodeValue		value;

  GPtrArray *		arguments;		// char *
  ViviCodeStatement *	body;
};

struct _ViviCodeFunctionClass
{
  ViviCodeValueClass	value_class;
};

GType			vivi_code_function_get_type   	(void);

ViviCodeValue *		vivi_code_function_new		(void);
ViviCodeValue *		vivi_code_function_new_from_script (SwfdecScript *	script);

void			vivi_code_function_set_body	(ViviCodeFunction *	function,
							 ViviCodeStatement *	body);
void			vivi_code_function_add_argument	(ViviCodeFunction *	function,
							 const char *		name);


G_END_DECLS
#endif
