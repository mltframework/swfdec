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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vivi_parser_scanner.h"

#include "vivi_parser_scanner_lex.h"

extern ViviParserScannerValue lex_value;
extern guint lex_line_number;
extern guint lex_column;
extern gsize lex_position;

G_DEFINE_TYPE (ViviParserScanner, vivi_parser_scanner, G_TYPE_OBJECT)

static void
vivi_parser_scanner_free_type_value (ViviParserScannerValue *value)
{
  switch (value->type) {
    case VALUE_TYPE_STRING:
      g_free (value->v_string);
      break;
    case VALUE_TYPE_IDENTIFIER:
      g_free (value->v_identifier);
      break;
    case VALUE_TYPE_ERROR:
      g_free (value->v_error);
      break;
    case VALUE_TYPE_NONE:
    case VALUE_TYPE_BOOLEAN:
    case VALUE_TYPE_NUMBER:
      /* nothing */
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  value->type = VALUE_TYPE_NONE;
}

static void
vivi_parser_scanner_dispose (GObject *object)
{
  ViviParserScanner *scanner = VIVI_PARSER_SCANNER (object);

  vivi_parser_scanner_free_type_value (&scanner->value);
  vivi_parser_scanner_free_type_value (&scanner->next_value);

  G_OBJECT_CLASS (vivi_parser_scanner_parent_class)->dispose (object);
}

static void
vivi_parser_scanner_class_init (ViviParserScannerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_parser_scanner_dispose;
}

static void
vivi_parser_scanner_init (ViviParserScanner *token)
{
}

static const struct {
  ViviParserScannerToken	token;
  const char *			name;
} token_names[] = {
  // special
  { TOKEN_NONE, "NONE" },
  { TOKEN_EOF, "EOF" },
  { TOKEN_ERROR, "ERROR" },
  { TOKEN_UNKNOWN, "UNKNOWN" },
  { TOKEN_LINE_TERMINATOR, "NEW LINE" },

  // comparision
  { TOKEN_BRACE_LEFT, "{", },
  { TOKEN_BRACE_RIGHT, "}", },
  { TOKEN_BRACKET_LEFT, "[", },
  { TOKEN_BRACKET_RIGHT, "]", },
  { TOKEN_PARENTHESIS_LEFT, "(", },
  { TOKEN_PARENTHESIS_RIGHT, ")", },

  // punctuation
  { TOKEN_DOT, ".", },
  { TOKEN_SEMICOLON, ";" },
  { TOKEN_COMMA, "," },

  // comparision
  { TOKEN_LESS_THAN, "<" },
  { TOKEN_GREATER_THAN, ">" },
  { TOKEN_LESS_THAN_OR_EQUAL, "<=" },
  { TOKEN_EQUAL_OR_GREATER_THAN, ">=" },

  // equality
  { TOKEN_EQUAL, "==" },
  { TOKEN_NOT_EQUAL, "!=" },
  { TOKEN_STRICT_EQUAL, "===" },
  { TOKEN_NOT_STRICT_EQUAL, "!==" },

  // operator
  { TOKEN_PLUS, "+" },
  { TOKEN_MINUS, "-" },
  { TOKEN_MULTIPLY, "*" },
  { TOKEN_DIVIDE, "/" },
  { TOKEN_REMAINDER, "%" },

  // shift
  { TOKEN_SHIFT_LEFT, "<<" },
  { TOKEN_SHIFT_RIGHT, ">>" },
  { TOKEN_SHIFT_RIGHT_UNSIGNED, ">>>" },

  // bitwise
  { TOKEN_BITWISE_AND, "&" },
  { TOKEN_BITWISE_OR, "|" },
  { TOKEN_BITWISE_XOR, "^" },
  
  // unary/postfix
  { TOKEN_LOGICAL_NOT, "!" },
  { TOKEN_BITWISE_NOT, "~" },
  { TOKEN_INCREASE, "++" },
  { TOKEN_DESCREASE, "--" },

  // conditional
  { TOKEN_QUESTION_MARK, "?" },
  { TOKEN_COLON, ":" },

  // logical
  { TOKEN_LOGICAL_AND, "&&" },
  { TOKEN_LOGICAL_OR, "||" },

  // assign
  { TOKEN_ASSIGN, "=" },
  { TOKEN_ASSIGN_MULTIPLY, "*=" },
  { TOKEN_ASSIGN_DIVIDE, "/=" },
  { TOKEN_ASSIGN_REMAINDER, "%=" },
  { TOKEN_ASSIGN_ADD, "+=" },
  { TOKEN_ASSIGN_MINUS, "-=" },
  { TOKEN_ASSIGN_SHIFT_LEFT, "<<=" },
  { TOKEN_ASSIGN_SHIFT_RIGHT, ">>=" },
  { TOKEN_ASSIGN_SHIFT_RIGHT_ZERO, ">>>=" },
  { TOKEN_ASSIGN_BITWISE_AND, "&=" },
  { TOKEN_ASSIGN_BITWISE_XOR, "^=" },
  { TOKEN_ASSIGN_BITWISE_OR, "|=" },

  // values
  { TOKEN_NULL, "NULL" },
  { TOKEN_BOOLEAN, "BOOLEAN" },
  { TOKEN_NUMBER, "NUMBER" },
  { TOKEN_STRING, "STRING" },
  { TOKEN_IDENTIFIER, "IDENTIFIER" },

  // keywords
  { TOKEN_BREAK, "break" },
  { TOKEN_CASE, "case" },
  { TOKEN_CATCH, "catch" },
  { TOKEN_CONTINUE, "continue" },
  { TOKEN_DEFAULT, "default" },
  { TOKEN_DELETE, "delete" },
  { TOKEN_DO, "do" },
  { TOKEN_ELSE, "else" },
  { TOKEN_FINALLY, "finally" },
  { TOKEN_FOR, "for" },
  { TOKEN_FUNCTION, "function" },
  { TOKEN_IF, "if" },
  { TOKEN_IN, "in" },
  { TOKEN_INSTANCEOF, "instanceof" },
  { TOKEN_NEW, "new" },
  { TOKEN_RETURN, "return" },
  { TOKEN_SWITCH, "switch" },
  { TOKEN_THIS, "this" },
  { TOKEN_THROW, "throw" },
  { TOKEN_TRY, "try" },
  { TOKEN_TYPEOF, "typeof" },
  { TOKEN_VAR, "var" },
  { TOKEN_VOID, "void" },
  { TOKEN_WHILE, "while" },
  { TOKEN_WITH, "with" },

  // reserved keywords
  { TOKEN_RESERVED_KEYWORD, "RESERVED KEYWORD" },

  // ActionScript specific
  { TOKEN_UNDEFINED, "undefined" },
  { TOKEN_TRACE, "trace" },

  { TOKEN_LAST, NULL }
};

const char *vivi_parser_scanner_token_name (ViviParserScannerToken token)
{
  int i;

  for (i = 0; token_names[i].token != TOKEN_LAST; i++) {
    if (token_names[i].token == token)
      return token_names[i].name;
  }

  g_assert_not_reached ();

  return "INVALID TOKEN";
}

static void
vivi_parser_scanner_advance (ViviParserScanner *scanner)
{
  g_return_if_fail (VIVI_IS_PARSER_SCANNER (scanner));

  vivi_parser_scanner_free_type_value (&scanner->value);

  scanner->token = scanner->next_token;
  scanner->value = scanner->next_value;
  scanner->line_terminator = scanner->next_line_terminator;
  scanner->line_number = scanner->next_line_number;
  scanner->column = scanner->next_column;
  scanner->position = scanner->next_position;

  while (scanner->waiting_errors != NULL) {
    if (scanner->error_handler != NULL) {
      scanner->error_handler (scanner->waiting_errors->data,
	  scanner->error_handler_data);
      g_free (scanner->waiting_errors->data);
    }
    scanner->waiting_errors = g_slist_delete_link (scanner->waiting_errors,
	scanner->waiting_errors);
  }

  if (scanner->file == NULL) {
    scanner->next_token = TOKEN_EOF;
    scanner->next_value.v_string = NULL;
  } else {
    do {
      scanner->next_token = yylex ();
      if (scanner->next_token == TOKEN_LINE_TERMINATOR) {
	scanner->next_line_terminator = TRUE;
	vivi_parser_scanner_free_type_value (&lex_value);
      } else if (scanner->next_token == TOKEN_ERROR) {
	scanner->waiting_errors = g_slist_prepend (scanner->waiting_errors,
	    lex_value.v_error);
	lex_value.type = VALUE_TYPE_NONE;
      }
    } while (scanner->next_token == TOKEN_LINE_TERMINATOR ||
	scanner->next_token == TOKEN_ERROR);

    scanner->next_value = lex_value;
    scanner->next_line_number = lex_line_number;
    scanner->next_column = lex_column;
    scanner->next_position = lex_position;

    lex_value.type = VALUE_TYPE_NONE;
  }
}

ViviParserScanner *
vivi_parser_scanner_new (FILE *file)
{
  ViviParserScanner *scanner;

  g_return_val_if_fail (file != NULL, NULL);

  scanner = g_object_new (VIVI_TYPE_PARSER_SCANNER, NULL);
  scanner->file = file;
  scanner->line_number = scanner->next_line_number = lex_line_number = 1;
  scanner->column = scanner->next_column = lex_column = 0;
  scanner->position = scanner->next_position = lex_position = 0;

  yyrestart (file);

  vivi_parser_scanner_advance (scanner);

  return scanner;
}

void
vivi_parser_scanner_set_error_handler (ViviParserScanner *scanner,
    ViviParserScannerFunction error_handler, gpointer user_data)
{
  scanner->error_handler = error_handler;
  scanner->error_handler_data = user_data;
}

ViviParserScannerToken
vivi_parser_scanner_get_next_token (ViviParserScanner *scanner)
{
  g_return_val_if_fail (VIVI_IS_PARSER_SCANNER (scanner), TOKEN_EOF);

  vivi_parser_scanner_advance (scanner);

  return scanner->token;
}

ViviParserScannerToken
vivi_parser_scanner_peek_next_token (ViviParserScanner *scanner)
{
  g_return_val_if_fail (VIVI_IS_PARSER_SCANNER (scanner), TOKEN_EOF);

  return scanner->next_token;
}

guint
vivi_parser_scanner_cur_line (ViviParserScanner *scanner)
{
  g_return_val_if_fail (VIVI_IS_PARSER_SCANNER (scanner), 0);

  return scanner->line_number;
}

guint
vivi_parser_scanner_cur_column (ViviParserScanner *scanner)
{
  g_return_val_if_fail (VIVI_IS_PARSER_SCANNER (scanner), 0);

  // TODO

  return 0;
}

char *
vivi_parser_scanner_get_line (ViviParserScanner *scanner)
{
  g_return_val_if_fail (VIVI_IS_PARSER_SCANNER (scanner), 0);

  // TODO

  return g_strdup ("");
}
