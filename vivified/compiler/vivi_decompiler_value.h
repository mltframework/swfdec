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

#ifndef _VIVI_DECOMPILER_VALUE_H_
#define _VIVI_DECOMPILER_VALUE_H_

#include <glib.h>

G_BEGIN_DECLS


typedef enum {
  VIVI_PRECEDENCE_NONE = 0,
  VIVI_PRECEDENCE_MEMBER,
  VIVI_PRECEDENCE_CALL,
  VIVI_PRECEDENCE_INCREMENT,
  VIVI_PRECEDENCE_UNARY,
  VIVI_PRECEDENCE_MULTIPLY,
  VIVI_PRECEDENCE_ADD,
  VIVI_PRECEDENCE_SHIFT,
  VIVI_PRECEDENCE_RELATIONAL,
  VIVI_PRECEDENCE_EQUALITY,
  VIVI_PRECEDENCE_BINARY_AND,
  VIVI_PRECEDENCE_BINARY_XOR,
  VIVI_PRECEDENCE_BINARY_OR,
  VIVI_PRECEDENCE_AND,
  VIVI_PRECEDENCE_OR,
  VIVI_PRECEDENCE_CONDITIONAL,
  VIVI_PRECEDENCE_ASSIGNMENT,
  VIVI_PRECEDENCE_COMMA
} ViviPrecedence;

typedef struct _ViviDecompilerValue ViviDecompilerValue;


void				vivi_decompiler_value_free		(ViviDecompilerValue *		value);
ViviDecompilerValue *		vivi_decompiler_value_new		(ViviPrecedence			precedence,
									 gboolean			constant,
									 char *			        text);
ViviDecompilerValue *		vivi_decompiler_value_new_printf      	(ViviPrecedence			precedence,
									 gboolean			constant,
									 const char *			format,
									 ...) G_GNUC_PRINTF (3, 4);
ViviDecompilerValue *		vivi_decompiler_value_copy		(const ViviDecompilerValue *	src);
const ViviDecompilerValue *	vivi_decompiler_value_get_undefined	(void);
ViviDecompilerValue *		vivi_decompiler_value_ensure_precedence	(ViviDecompilerValue *          value,
									 ViviPrecedence			precedence);

const char *			vivi_decompiler_value_get_text		(const ViviDecompilerValue *	value);
ViviPrecedence			vivi_decompiler_value_get_precedence  	(const ViviDecompilerValue *	value);
gboolean			vivi_decompiler_value_is_constant     	(const ViviDecompilerValue *	value);

G_END_DECLS
#endif
