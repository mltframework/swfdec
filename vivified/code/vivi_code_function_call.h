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

#ifndef _VIVI_CODE_FUNCTION_CALL_H_
#define _VIVI_CODE_FUNCTION_CALL_H_

#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviCodeFunctionCall ViviCodeFunctionCall;
typedef struct _ViviCodeFunctionCallClass ViviCodeFunctionCallClass;

#define VIVI_TYPE_CODE_FUNCTION_CALL                    (vivi_code_function_call_get_type())
#define VIVI_IS_CODE_FUNCTION_CALL(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_FUNCTION_CALL))
#define VIVI_IS_CODE_FUNCTION_CALL_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_FUNCTION_CALL))
#define VIVI_CODE_FUNCTION_CALL(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_FUNCTION_CALL, ViviCodeFunctionCall))
#define VIVI_CODE_FUNCTION_CALL_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_FUNCTION_CALL, ViviCodeFunctionCallClass))
#define VIVI_CODE_FUNCTION_CALL_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_FUNCTION_CALL, ViviCodeFunctionCallClass))

struct _ViviCodeFunctionCall
{
  ViviCodeValue		parent;

  gboolean		construct;	/* This is a constructor call */
  ViviCodeValue *	value;		/* value to call function with */
  ViviCodeValue *	name;		/* property to get */
  GPtrArray *		arguments;	/* array of ViviCodeValue containing the function's arguments */
};

struct _ViviCodeFunctionCallClass
{
  ViviCodeValueClass	value_class;
};

GType			vivi_code_function_call_get_type   	(void);

ViviCodeValue *		vivi_code_function_call_new		(ViviCodeValue *	value,
								 ViviCodeValue *	name);

void			vivi_code_function_call_set_construct	(ViviCodeFunctionCall * call,
								 gboolean		construct);
void			vivi_code_function_call_add_argument	(ViviCodeFunctionCall *	call,
								 ViviCodeValue *	argument);

G_END_DECLS
#endif
