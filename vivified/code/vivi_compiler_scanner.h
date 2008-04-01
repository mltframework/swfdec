/* Vivified
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _VIVI_COMPILER_SCANNER_H_
#define _VIVI_COMPILER_SCANNER_H_

#include <stdio.h>
#include <swfdec/swfdec.h>

G_BEGIN_DECLS


typedef enum {
  // special
  TOKEN_NONE = 0,
  TOKEN_EOF,
  TOKEN_UNKNOWN,

  // comparision
  TOKEN_BRACE_LEFT,
  TOKEN_BRACE_RIGHT,
  TOKEN_BRACKET_LEFT,
  TOKEN_BRACKET_RIGHT,
  TOKEN_PARENTHESIS_LEFT,
  TOKEN_PARENTHESIS_RIGHT,

  // punctuation
  TOKEN_DOT,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,

  // comparision
  TOKEN_LESS_THAN,
  TOKEN_GREATER_THAN,
  TOKEN_LESS_THAN_OR_EQUAL,
  TOKEN_EQUAL_OR_MORE_THAN,

  // equality
  TOKEN_EQUAL,
  TOKEN_NOT_EQUAL,
  TOKEN_STRICT_EQUAL,
  TOKEN_NOT_STRICT_EQUAL,

  // operator
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,
  TOKEN_REMAINDER,

  // shift
  TOKEN_SHIFT_LEFT,
  TOKEN_SHIFT_RIGHT,
  TOKEN_SHIFT_RIGHT_UNSIGNED,

  // bitwise
  TOKEN_BITWISE_AND,
  TOKEN_BITWISE_OR,
  TOKEN_BITWISE_XOR,
  
  // unary/postfix
  TOKEN_LOGICAL_NOT,
  TOKEN_BITWISE_NOT,
  TOKEN_PLUSPLUS,
  TOKEN_MINUSMINUS,

  // conditional
  TOKEN_QUESTION_MARK,
  TOKEN_COLON,

  // logical
  TOKEN_LOGICAL_AND,
  TOKEN_LOGICAL_OR,

  // assign
  TOKEN_ASSIGN,
  TOKEN_ASSIGN_MULTIPLY,
  TOKEN_ASSIGN_DIVIDE,
  TOKEN_ASSIGN_REMAINDER,
  TOKEN_ASSIGN_ADD,
  TOKEN_ASSIGN_MINUS,
  TOKEN_ASSIGN_SHIFT_LEFT,
  TOKEN_ASSIGN_SHIFT_RIGHT,
  TOKEN_ASSIGN_SHIFT_RIGHT_ZERO,
  TOKEN_ASSIGN_BITWISE_AND,
  TOKEN_ASSIGN_BITWISE_XOR,
  TOKEN_ASSIGN_BITWISE_OR,

  // values
  TOKEN_NULL,
  TOKEN_BOOLEAN,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_IDENTIFIER,

  // keywords
  TOKEN_BREAK,
  TOKEN_CASE,
  TOKEN_CATCH,
  TOKEN_CONTINUE,
  TOKEN_DEFAULT,
  TOKEN_DELETE,
  TOKEN_DO,
  TOKEN_ELSE,
  TOKEN_FINALLY,
  TOKEN_FOR,
  TOKEN_FUNCTION,
  TOKEN_IF,
  TOKEN_IN,
  TOKEN_INSTANCEOF,
  TOKEN_NEW,
  TOKEN_RETURN,
  TOKEN_SWITCH,
  TOKEN_THIS,
  TOKEN_THROW,
  TOKEN_TRY,
  TOKEN_TYPEOF,
  TOKEN_VAR,
  TOKEN_VOID,
  TOKEN_WITH,

  // reserved keywords
  TOKEN_FUTURE,

  TOKEN_LAST
} ViviCompilerScannerToken;

typedef union {
  gboolean	v_boolean;
  double	v_number;
  char *	v_string;
  char *	v_identifier;
} ViviCompilerScannerValue;

typedef struct _ViviCompilerScanner ViviCompilerScanner;
typedef struct _ViviCompilerScannerClass ViviCompilerScannerClass;

#define VIVI_TYPE_COMPILER_SCANNER                    (vivi_compiler_scanner_get_type())
#define VIVI_IS_COMPILER_SCANNER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_COMPILER_SCANNER))
#define VIVI_IS_COMPILER_SCANNER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_COMPILER_SCANNER))
#define VIVI_COMPILER_SCANNER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_COMPILER_SCANNER, ViviCompilerScanner))
#define VIVI_COMPILER_SCANNER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_COMPILER_SCANNER, ViviCompilerScannerClass))
#define VIVI_COMPILER_SCANNER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_COMPILER_SCANNER, ViviCompilerScannerClass))

struct _ViviCompilerScanner
{
  GObject			object;

  FILE *			file;

  ViviCompilerScannerToken	token;
  ViviCompilerScannerToken	next_token;
  ViviCompilerScannerValue	value;
  ViviCompilerScannerValue	next_value;
};

struct _ViviCompilerScannerClass
{
  GObjectClass		object_class;
};

GType				vivi_compiler_scanner_get_type   	(void);

ViviCompilerScanner *		vivi_compiler_scanner_new		(FILE *		file);
ViviCompilerScannerToken	vivi_compiler_scanner_get_next_token	(ViviCompilerScanner *	scanner);
ViviCompilerScannerToken	vivi_compiler_scanner_peek_next_token	(ViviCompilerScanner *	scanner);

const char *			vivi_compiler_scanner_token_name	(ViviCompilerScannerToken token);


G_END_DECLS
#endif
