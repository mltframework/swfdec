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

#ifndef _VIVI_CODE_VALUE_H_
#define _VIVI_CODE_VALUE_H_

#include <vivified/code/vivi_code_token.h>

G_BEGIN_DECLS


typedef enum {
  VIVI_PRECEDENCE_COMMA,
  VIVI_PRECEDENCE_ASSIGNMENT,
  VIVI_PRECEDENCE_CONDITIONAL,
  VIVI_PRECEDENCE_OR,
  VIVI_PRECEDENCE_AND,
  VIVI_PRECEDENCE_BINARY_OR,
  VIVI_PRECEDENCE_BINARY_XOR,
  VIVI_PRECEDENCE_BINARY_AND,
  VIVI_PRECEDENCE_EQUALITY,
  VIVI_PRECEDENCE_RELATIONAL,
  VIVI_PRECEDENCE_SHIFT,
  VIVI_PRECEDENCE_ADD,
  VIVI_PRECEDENCE_MULTIPLY,
  VIVI_PRECEDENCE_UNARY,
  VIVI_PRECEDENCE_INCREMENT,
  VIVI_PRECEDENCE_CALL,
  VIVI_PRECEDENCE_MEMBER,
  VIVI_PRECEDENCE_PARENTHESIS
} ViviPrecedence;

#define VIVI_PRECEDENCE_MIN VIVI_PRECEDENCE_COMMA
#define VIVI_PRECEDENCE_MAX VIVI_PRECEDENCE_PARENTHESIS

typedef struct _ViviCodeValue ViviCodeValue;
typedef struct _ViviCodeValueClass ViviCodeValueClass;

#define VIVI_TYPE_CODE_VALUE                    (vivi_code_value_get_type())
#define VIVI_IS_CODE_VALUE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_VALUE))
#define VIVI_IS_CODE_VALUE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_VALUE))
#define VIVI_CODE_VALUE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_VALUE, ViviCodeValue))
#define VIVI_CODE_VALUE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_VALUE, ViviCodeValueClass))
#define VIVI_CODE_VALUE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_VALUE, ViviCodeValueClass))

struct _ViviCodeValue
{
  ViviCodeToken		token;

  ViviPrecedence	precedence;
};

struct _ViviCodeValueClass
{
  ViviCodeTokenClass  	token_class;

  gboolean		(* is_constant)			(ViviCodeValue *	value);
  ViviCodeValue *	(* optimize)			(ViviCodeValue *	value,
							 SwfdecAsValueType	hint);
};

GType			vivi_code_value_get_type   	(void);

gboolean		vivi_code_value_is_constant	(ViviCodeValue *	value);
gboolean		vivi_code_value_is_equal	(ViviCodeValue *	a,
							 ViviCodeValue *	b);

void			vivi_code_value_set_precedence	(ViviCodeValue *	value,
							 ViviPrecedence		precedence);
ViviPrecedence		vivi_code_value_get_precedence	(ViviCodeValue *	value);

ViviCodeValue *		vivi_code_value_optimize	(ViviCodeValue *	value,
							 SwfdecAsValueType	hint);


G_END_DECLS
#endif
