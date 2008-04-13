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

#ifndef DEFAULT_BINARY
#define DEFAULT_BINARY(CapsName, underscore_name, operator_name, bytecode, precedence)
#endif

DEFAULT_BINARY (Add,		add,		"add",	SWFDEC_AS_ACTION_ADD,		VIVI_PRECEDENCE_ADD)
DEFAULT_BINARY (Subtract,	subtract,	"-",	SWFDEC_AS_ACTION_SUBTRACT,	VIVI_PRECEDENCE_ADD)
DEFAULT_BINARY (Multiply,	multiply,	"*",	SWFDEC_AS_ACTION_MULTIPLY,	VIVI_PRECEDENCE_MULTIPLY)
DEFAULT_BINARY (Divide,		divide,		"/",	SWFDEC_AS_ACTION_DIVIDE,	VIVI_PRECEDENCE_MULTIPLY)
DEFAULT_BINARY (Modulo,		modulo,		"%",	SWFDEC_AS_ACTION_MODULO,	VIVI_PRECEDENCE_MODULO)
DEFAULT_BINARY (Equals,		equals,		"==",	SWFDEC_AS_ACTION_EQUALS,	VIVI_PRECEDENCE_EQUALITY)
DEFAULT_BINARY (Less,		less,		"<",	SWFDEC_AS_ACTION_LESS,		VIVI_PRECEDENCE_RELATIONAL)
DEFAULT_BINARY (LogicalAnd,	logical_and,	"and",	SWFDEC_AS_ACTION_AND,		VIVI_PRECEDENCE_AND)
DEFAULT_BINARY (LogicalOr,	logical_or,	"or",	SWFDEC_AS_ACTION_OR,		VIVI_PRECEDENCE_OR)
DEFAULT_BINARY (StringEquals,	string_equals,	"eq",	SWFDEC_AS_ACTION_STRING_EQUALS,	VIVI_PRECEDENCE_EQUALITY)
DEFAULT_BINARY (StringLess,	string_less,	"lt",	SWFDEC_AS_ACTION_STRING_LESS,	VIVI_PRECEDENCE_RELATIONAL)
DEFAULT_BINARY (Add2,		add2,	  	"+",	SWFDEC_AS_ACTION_ADD2,		VIVI_PRECEDENCE_ADD)
DEFAULT_BINARY (Less2,		less2,		"<",	SWFDEC_AS_ACTION_LESS2,		VIVI_PRECEDENCE_RELATIONAL)
DEFAULT_BINARY (Equals2,	equals2,	"==",	SWFDEC_AS_ACTION_EQUALS2,	VIVI_PRECEDENCE_EQUALITY)
DEFAULT_BINARY (BitwiseAnd,	bitwise_and,	"&",	SWFDEC_AS_ACTION_BIT_AND,	VIVI_PRECEDENCE_BINARY_AND)
DEFAULT_BINARY (BitwiseOr,	bitwise_or,	"|",	SWFDEC_AS_ACTION_BIT_OR,	VIVI_PRECEDENCE_BINARY_OR)
DEFAULT_BINARY (BitwiseXor,	bitwise_xor,	"^",	SWFDEC_AS_ACTION_BIT_XOR,	VIVI_PRECEDENCE_BINARY_XOR)
DEFAULT_BINARY (LeftShift,	left_shift,	"<<",	SWFDEC_AS_ACTION_BIT_LSHIFT,	VIVI_PRECEDENCE_SHIFT)
DEFAULT_BINARY (RightShift,	right_shift,	">>",	SWFDEC_AS_ACTION_BIT_RSHIFT,	VIVI_PRECEDENCE_SHIFT)
DEFAULT_BINARY (UnsignedRightShift,unsigned_right_shift,">>>",SWFDEC_AS_ACTION_BIT_URSHIFT,VIVI_PRECEDENCE_SHIFT)
DEFAULT_BINARY (StrictEquals,	strict_equals,	"===",	SWFDEC_AS_ACTION_STRICT_EQUALS,	VIVI_PRECEDENCE_EQUALITY)
DEFAULT_BINARY (Greater,	greater,	">",	SWFDEC_AS_ACTION_GREATER,	VIVI_PRECEDENCE_RELATIONAL)
DEFAULT_BINARY (StringGreater,	string_greater,	"gt",	SWFDEC_AS_ACTION_STRING_GREATER,VIVI_PRECEDENCE_RELATIONAL)
/* other special ones:
DEFAULT_BINARY (And,		and,		"&&",	???,				VIVI_PRECEDENCE_AND)
DEFAULT_BINARY (Or,		or,		"||",	???,				VIVI_PRECEDENCE_AND)
*/

#undef DEFAULT_BINARY
