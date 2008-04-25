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

G_DEFINE_TYPE (ViviParserScanner, vivi_parser_scanner, G_TYPE_OBJECT)

static void
vivi_parser_value_reset (ViviParserValue *value)
{
  if (value->token == TOKEN_STRING) {
    g_free (value->value.v_string);
  } else if (value->token == TOKEN_IDENTIFIER) {
    g_free (value->value.v_identifier);
  } else if (value->token == TOKEN_ERROR) {
    g_free (value->value.v_error);
  }

  /* FIXME: do a memset 0 here? */
  value->token = TOKEN_NONE;
}

static void
vivi_parser_scanner_pop (ViviParserScanner *scanner)
{
  ViviParserValue *value;

  /* ensure there is a value */
  vivi_parser_scanner_get_value (scanner, 0);
  value = swfdec_ring_buffer_pop (scanner->values);

  vivi_parser_value_reset (value);
}

static void
vivi_parser_scanner_dispose (GObject *object)
{
  ViviParserScanner *scanner = VIVI_PARSER_SCANNER (object);

  while (swfdec_ring_buffer_get_n_elements (scanner->values))
    vivi_parser_scanner_pop (scanner);

  vivi_parser_scanner_lex_destroy (scanner->scanner);

  G_OBJECT_CLASS (vivi_parser_scanner_parent_class)->dispose (object);
}

static void
vivi_parser_scanner_class_init (ViviParserScannerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_parser_scanner_dispose;
}

static void
vivi_parser_scanner_init (ViviParserScanner *scanner)
{
  vivi_parser_scanner_lex_init_extra (scanner, &scanner->scanner);
  scanner->values = swfdec_ring_buffer_new_for_type (ViviParserValue, 4);
}

static const struct {
  ViviParserScannerToken	token;
  const char *			name;
} token_names[] = {
  // special
  { TOKEN_NONE, "NONE" },
  { TOKEN_ERROR, "ERROR" },
  { TOKEN_UNKNOWN, "UNKNOWN" },

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
  ViviParserValue *value;

  g_return_if_fail (VIVI_IS_PARSER_SCANNER (scanner));

  value = swfdec_ring_buffer_push (scanner->values);
  if (value == NULL) {
    swfdec_ring_buffer_set_size (scanner->values,
	swfdec_ring_buffer_get_size (scanner->values) + 4);
    value = swfdec_ring_buffer_push (scanner->values);
  }

  if (scanner->file == NULL) {
    value->token = TOKEN_NONE;
    value->column = 0;
    value->position = 0;
    value->line_number = 0;
    value->line_terminator = FALSE;
  } else {
    value->line_terminator = FALSE;
    for (;;) {
      value->token = vivi_parser_scanner_lex (scanner->scanner, value);
      g_print ("got %s\n", vivi_parser_scanner_token_name (value->token));
      if (value->token == TOKEN_ERROR) {
	vivi_parser_scanner_error (scanner, 0, 0, "%s", value->value.v_error);
	vivi_parser_value_reset (value);
      } else {
	break;
      }
    }
    value->line_number = vivi_parser_scanner_get_lineno (scanner->scanner);
    value->column = 0; /* FIXME */
    value->position = 0; /* FIXME */
  }
}

const ViviParserValue *
vivi_parser_scanner_get_value (ViviParserScanner *scanner, guint i)
{
  g_return_val_if_fail (VIVI_IS_PARSER_SCANNER (scanner), NULL);

  while (swfdec_ring_buffer_get_n_elements (scanner->values) <= i) {
    vivi_parser_scanner_advance (scanner);
  }
  return swfdec_ring_buffer_peek_nth (scanner->values, i);
}

ViviParserScanner *
vivi_parser_scanner_new (FILE *file)
{
  ViviParserScanner *scanner;

  g_return_val_if_fail (file != NULL, NULL);

  scanner = g_object_new (VIVI_TYPE_PARSER_SCANNER, NULL);
  scanner->file = file;

  vivi_parser_scanner_restart (file, scanner->scanner);

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
  g_return_val_if_fail (VIVI_IS_PARSER_SCANNER (scanner), TOKEN_NONE);

  vivi_parser_scanner_advance (scanner);

  return vivi_parser_scanner_get_value (scanner, 0)->token;
}

ViviParserScannerToken
vivi_parser_scanner_peek_next_token (ViviParserScanner *scanner)
{
  g_return_val_if_fail (VIVI_IS_PARSER_SCANNER (scanner), TOKEN_NONE);

  return vivi_parser_scanner_get_value (scanner, 1)->token;
}

void
vivi_parser_scanner_error (ViviParserScanner *scanner, 
    guint line, int column, const char *format, ...)
{
  va_list args;

  g_return_if_fail (VIVI_IS_PARSER_SCANNER (scanner));
  g_return_if_fail (column >= -1);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  vivi_parser_scanner_errorv (scanner, line, column, format, args);
  va_end (args);
}

void
vivi_parser_scanner_errorv (ViviParserScanner *scanner, 
    guint line, int column, const char *format, va_list args)
{
  char *message;

  g_return_if_fail (VIVI_IS_PARSER_SCANNER (scanner));
  g_return_if_fail (column >= -1);
  g_return_if_fail (format != NULL);

  message = g_strdup_vprintf (format, args);

  if (column >= 0) {
    g_printerr ("%u,%i: error: %s\n", line, column, message);
  } else {
    g_printerr ("%u: error: %s\n", line, message);
  }

  g_free (message);
}
