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

#include <string.h>

#include "vivi_compiler.h"

#include "vivi_compiler_scanner.h"

#include "vivi_code_assignment.h"
#include "vivi_code_binary.h"
#include "vivi_code_block.h"
#include "vivi_code_break.h"
#include "vivi_code_constant.h"
#include "vivi_code_continue.h"
#include "vivi_code_function.h"
#include "vivi_code_function_call.h"
#include "vivi_code_get.h"
#include "vivi_code_get_url.h"
#include "vivi_code_goto.h"
#include "vivi_code_if.h"
#include "vivi_code_init_object.h"
#include "vivi_code_loop.h"
#include "vivi_code_return.h"
#include "vivi_code_trace.h"
#include "vivi_code_unary.h"
#include "vivi_code_value_statement.h"
#include "vivi_compiler_empty_statement.h"
#include "vivi_compiler_get_temporary.h"


#include "vivi_code_text_printer.h"

enum {
  ERROR_TOKEN_LITERAL = TOKEN_LAST + 1,
  ERROR_TOKEN_PROPERTY_NAME,
  ERROR_TOKEN_PRIMARY_EXPRESSION,
  ERROR_TOKEN_EXPRESSION_STATEMENT,
  ERROR_TOKEN_STATEMENT
};

static const struct {
  int				token;
  const char *			name;
} error_names[] = {
  { ERROR_TOKEN_LITERAL, "LITERAL" },
  { ERROR_TOKEN_PROPERTY_NAME, "PROPERTY NAME" },
  { ERROR_TOKEN_PRIMARY_EXPRESSION, "PRIMARY EXPRESSION" },
  { ERROR_TOKEN_EXPRESSION_STATEMENT, "EXPRESSION STATEMENT" },
  { ERROR_TOKEN_STATEMENT, "STATEMENT" },
  { TOKEN_LAST, NULL }
};

#define FAIL(x) ((x) < 0 ? -(x) : (x))

typedef int (*ParseValueFunction) (ViviCompilerScanner *scanner, ViviCodeValue **value);
typedef int (*ParseValueStatementFunction) (ViviCompilerScanner *scanner, ViviCodeValue **value, ViviCodeStatement **statement);
typedef int (*ParseStatementFunction) (ViviCompilerScanner *scanner, ViviCodeStatement **statement);

static int
parse_statement_list (ViviCompilerScanner *scanner, ParseStatementFunction function, ViviCodeStatement **statement, guint separator);
static int
parse_value_list (ViviCompilerScanner *scanner, ParseValueFunction function,
    ViviCodeValue ***list, guint separator);

// helpers

static gboolean
check_token (ViviCompilerScanner *scanner, ViviCompilerScannerToken token)
{
  vivi_compiler_scanner_peek_next_token (scanner);
  if (scanner->next_token != token)
    return FALSE;
  vivi_compiler_scanner_get_next_token (scanner);
  return TRUE;
}

static void
free_value_list (ViviCodeValue **list)
{
  int i;

  for (i = 0; list[i] != NULL; i++) {
    g_object_unref (list[i]);
  }
  g_free (list);
}

static ViviCodeStatement *
vivi_compiler_combine_statements (guint count, ...)
{
  va_list args;
  ViviCodeBlock *block;
  guint i;

  if (count == 0)
    return NULL;

  va_start (args, count);
  block = NULL;
  for (i = 0; i < count; i++) {
    ViviCodeStatement *statement = va_arg (args, ViviCodeStatement *);

    if (statement == NULL)
      continue;

    g_assert (VIVI_IS_CODE_STATEMENT (statement));

    if (block == NULL) {
      if (VIVI_IS_CODE_BLOCK (statement)) {
	block = VIVI_CODE_BLOCK (statement);
	continue;
      } else {
	block = VIVI_CODE_BLOCK (vivi_code_block_new ());
      }
    }

    vivi_code_block_add_statement (block, statement);
    g_object_unref (statement);
  }
  va_end (args);

  return VIVI_CODE_STATEMENT (block);
}

static ViviCodeStatement *
vivi_compiler_assignment_new (ViviCodeValue *left, ViviCodeValue *right)
{
  ViviCodeValue *from;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (left), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (right), NULL);

  from = NULL;

  if (VIVI_IS_CODE_GET (left)) {
    ViviCodeGet *get = VIVI_CODE_GET (left);

    if (get->from != NULL) {
      from = g_object_ref (get->from);
      left = g_object_ref (get->name);
    }
  }

  if (VIVI_IS_CODE_GET (left)) {
    ViviCodeGet *get = VIVI_CODE_GET (left);

    if (get->from == NULL)
      left = g_object_ref (get->name);
  }

  return vivi_code_assignment_new (from, left, right);
}

static ViviCodeValue *
vivi_compiler_get_new (ViviCodeValue *from, ViviCodeValue *name)
{
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (from), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (name), NULL);

  if (VIVI_IS_CODE_GET (name)) {
    ViviCodeGet *get = VIVI_CODE_GET (name);

    if (get->from == NULL)
      name = g_object_ref (get->name);
  }

  return vivi_code_get_new (from, name);
}

// values

static int
parse_null_literal (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, TOKEN_NULL))
    return -TOKEN_NULL;

  *value = vivi_code_constant_new_null ();
  return TOKEN_NONE;
}

static int
parse_boolean_literal (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, TOKEN_BOOLEAN))
    return -TOKEN_BOOLEAN;

  *value = vivi_code_constant_new_boolean (scanner->value.v_boolean);
  return TOKEN_NONE;
}

static int
parse_numeric_literal (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, TOKEN_NUMBER))
    return -TOKEN_NUMBER;

  *value = vivi_code_constant_new_number (scanner->value.v_number);
  return TOKEN_NONE;
}

static int
parse_string_literal (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, TOKEN_STRING))
    return -TOKEN_STRING;

  *value = vivi_code_constant_new_string (scanner->value.v_string);
  return TOKEN_NONE;
}

static int
parse_literal (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  int i, expected;
  ParseValueFunction functions[] = {
    parse_null_literal,
    parse_boolean_literal,
    parse_numeric_literal,
    parse_string_literal,
    NULL
  };

  *value = NULL;

  for (i = 0; functions[i] != NULL; i++) {
    expected = functions[i] (scanner, value);
    if (expected >= 0)
      return expected;
  }

  return -ERROR_TOKEN_LITERAL;
}

static int
parse_identifier (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, TOKEN_IDENTIFIER))
    return -TOKEN_IDENTIFIER;

  *value = vivi_code_get_new_name (scanner->value.v_identifier);

  return TOKEN_NONE;
}

static int
parse_property_name (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  int i, expected;
  ParseValueFunction functions[] = {
    parse_identifier,
    parse_string_literal,
    parse_numeric_literal,
    NULL
  };

  *value = NULL;

  for (i = 0; functions[i] != NULL; i++) {
    expected = functions[i] (scanner, value);
    if (expected >= 0)
      return expected;
  }

  return -ERROR_TOKEN_PROPERTY_NAME;
}

static int
parse_assignment_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement);

static int
parse_object_literal (ViviCompilerScanner *scanner, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  int expected;

  *value = NULL;
  *statement = NULL;

  if (!check_token (scanner, TOKEN_BRACE_LEFT))
    return -TOKEN_BRACE_LEFT;

  *value = vivi_code_init_object_new ();

  if (!check_token (scanner, TOKEN_BRACE_RIGHT)) {
    do {
      ViviCodeValue *property, *initializer;
      ViviCodeStatement *statement_new;

      expected = parse_property_name (scanner, &property);
      if (expected != TOKEN_NONE) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL (expected);
      }

      if (!check_token (scanner, TOKEN_COLON)) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return TOKEN_COLON;
      }

      expected = parse_assignment_expression (scanner, &initializer,
	  &statement_new);
      if (expected != TOKEN_NONE) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL (expected);
      }

      *statement =
	vivi_compiler_combine_statements (2, *statement, statement_new);

      vivi_code_init_object_add_variable (VIVI_CODE_INIT_OBJECT (*value),
	  property, initializer);
    } while (check_token (scanner, TOKEN_COMMA));
  }

  if (!check_token (scanner, TOKEN_BRACE_RIGHT)) {
    g_object_unref (*value);
    *value = NULL;
    if (*statement != NULL) {
      g_object_unref (*statement);
      *statement = NULL;
    }
    return TOKEN_BRACE_RIGHT;
  }

  return TOKEN_NONE;
}

// misc

static int
parse_variable_declaration (ViviCompilerScanner *scanner,
    ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *identifier, *value;
  ViviCodeStatement *assignment, *statement_right;

  *statement = NULL;

  expected = parse_identifier (scanner, &identifier);
  if (expected != TOKEN_NONE)
    return expected;

  if (check_token (scanner, TOKEN_ASSIGN)) {
    expected = parse_assignment_expression (scanner, &value, &statement_right);
    if (expected != TOKEN_NONE) {
      g_object_unref (identifier);
      return FAIL (expected);
    }
  } else {
    value = vivi_code_constant_new_undefined ();
    statement_right = NULL;
  }

  assignment = vivi_compiler_assignment_new (identifier, value);
  vivi_code_assignment_set_local (VIVI_CODE_ASSIGNMENT (assignment), TRUE);

  *statement =
    vivi_compiler_combine_statements (2, statement_right, assignment);

  return TOKEN_NONE;
}

// expression

static int
parse_expression (ViviCompilerScanner *scanner, ViviCodeValue **value, ViviCodeStatement **statement);

static int
parse_primary_expression (ViviCompilerScanner *scanner, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  int i, expected;
  ParseValueFunction functions[] = {
    parse_identifier,
    parse_literal,
    //parse_array_literal,
    NULL
  };

  *value = NULL;
  *statement = NULL;

  if (check_token (scanner, TOKEN_THIS)) {
    *value = vivi_code_get_new_name ("this");
    return TOKEN_NONE;
  }

  if (check_token (scanner, TOKEN_PARENTHESIS_LEFT)) {
    expected = parse_expression (scanner, value, statement);
    if (expected != TOKEN_NONE)
      return FAIL (expected);
    if (!check_token (scanner, TOKEN_PARENTHESIS_RIGHT)) {
      g_object_unref (*value);
      *value = NULL;
      return TOKEN_PARENTHESIS_RIGHT;
    }
    return TOKEN_NONE;
  }


  expected = parse_object_literal (scanner, value, statement);
  if (expected == TOKEN_NONE || expected >= 0)
    return expected;

  for (i = 0; functions[i] != NULL; i++) {
    expected = functions[i] (scanner, value);
    if (expected >= 0)
      return expected;
  }

  return -ERROR_TOKEN_PRIMARY_EXPRESSION;
}

static int
parse_member_expression (ViviCompilerScanner *scanner, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *member;
  ViviCodeStatement *statement_member;

  *value = NULL;
  *statement = NULL;

  // TODO: new MemberExpression Arguments

  expected = parse_primary_expression (scanner, value, statement);
  //if (expected == STATUS_CANCEL)
  //  expected = parse_function_expression (scanner, value);

  if (expected != TOKEN_NONE)
    return expected;

  do {
    ViviCodeValue *tmp;

    if (check_token (scanner, TOKEN_BRACKET_LEFT)) {
      expected = parse_expression (scanner, &member, &statement_member);
      if (expected != TOKEN_NONE) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL (expected);
      }

      *statement = vivi_compiler_combine_statements (2, *statement,
	  statement_member);

      if (!check_token (scanner, TOKEN_BRACKET_RIGHT)) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return TOKEN_BRACKET_RIGHT;
      }
    } else if (check_token (scanner, TOKEN_DOT)) {
      expected = parse_identifier (scanner, &member);
      if (expected != TOKEN_NONE)
	return FAIL (expected);
    } else {
      return TOKEN_NONE;
    }

    tmp = *value;
    *value = vivi_compiler_get_new (tmp, member);
    g_object_unref (tmp);
    g_object_unref (member);
  } while (TRUE);

  g_assert_not_reached ();
}

static int
parse_new_expression (ViviCompilerScanner *scanner, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  int expected;

  *value = NULL;
  *statement = NULL;

  if (check_token (scanner, TOKEN_NEW)) {
    expected = parse_new_expression (scanner, value, statement);
    if (expected != TOKEN_NONE)
      return FAIL (expected);
    if (!VIVI_IS_CODE_FUNCTION_CALL (*value)) {
      ViviCodeValue *tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_code_function_call_new (NULL, tmp);
      g_object_unref (tmp);
    }
    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (*value),
	TRUE);
    return TOKEN_NONE;
  } else {
    return parse_member_expression (scanner, value, statement);
  }
}

static int
parse_left_hand_side_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  int expected;

  *value = NULL;

  expected = parse_new_expression (scanner, value, statement);
  //if (expected == STATUS_CANCEL)
  //  expected = parse_call_expression (scanner, value);

  return expected;
}

static int
parse_postfix_expression (ViviCompilerScanner *scanner, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *operation, *one, *temporary;
  const char *operator;

  *value = NULL;
  *statement = NULL;

  expected = parse_left_hand_side_expression (scanner, value, statement);
  if (expected != TOKEN_NONE)
    return expected;

  // FIXME: Don't allow new line here

  if (check_token (scanner, TOKEN_INCREASE)) {
    operator = "+";
  } else if (check_token (scanner, TOKEN_DESCREASE)) {
    operator = "-";
  } else {
    return TOKEN_NONE;
  }

  if (!VIVI_IS_CODE_GET (*value)) {
    g_printerr ("Invalid INCREASE/DECREASE\n");
    return -scanner->token;
  }

  one = vivi_code_constant_new_number (1);
  operation = vivi_code_binary_new_name (*value, one, operator);
  g_object_unref (one);

  temporary = vivi_compiler_get_temporary_new ();
  *statement = vivi_compiler_combine_statements (3, *statement,
      vivi_compiler_assignment_new (temporary, *value),
      vivi_compiler_assignment_new (*value, operation));
  g_object_unref (operation);

  g_object_unref (*value);
  *value = temporary;

  return TOKEN_NONE;
}

static int
parse_unary_expression (ViviCompilerScanner *scanner, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *tmp, *one;
  const char *operator;

  *value = NULL;

  operator = NULL;

  vivi_compiler_scanner_peek_next_token (scanner);
  switch ((int)scanner->next_token) {
    /*case TOKEN_DELETE:
    case TOKEN_VOID:
    case TOKEN_TYPEOF:*/
    case TOKEN_INCREASE:
      operator = "+";
      // fall through
    case TOKEN_DESCREASE:
      if (!operator) operator = "-";

      vivi_compiler_scanner_get_next_token (scanner);
      expected = parse_unary_expression (scanner, value, statement);
      if (expected != TOKEN_NONE)
	return FAIL (expected);

      if (!VIVI_IS_CODE_GET (*value)) {
	g_printerr ("Invalid INCREASE/DECREASE\n");
	return -scanner->token;
      }

      one = vivi_code_constant_new_number (1);
      tmp = vivi_code_binary_new_name (*value, one, operator);
      g_object_unref (one);

      *statement = vivi_compiler_combine_statements (2, *statement,
	  vivi_compiler_assignment_new (*value, tmp));
      g_object_unref (tmp);

      return TOKEN_NONE;
    /*case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BITWISE_NOT:*/
    case TOKEN_LOGICAL_NOT:
      vivi_compiler_scanner_get_next_token (scanner);
      expected = parse_unary_expression (scanner, value, statement);
      if (expected != TOKEN_NONE)
	return FAIL (expected);
      tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_code_unary_new (tmp, '!');
      g_object_unref (tmp);
      return TOKEN_NONE;
    default:
      return parse_postfix_expression (scanner, value, statement);
  }
}

typedef enum {
  PASS_ALWAYS,
  PASS_LOGICAL_OR,
  PASS_LOGICAL_AND
} ParseOperatorPass;

static int
parse_operator_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement,
    const ViviCompilerScannerToken *tokens, ParseOperatorPass pass,
    ParseValueStatementFunction next_parse_function)
{
  int expected, i;
  ViviCodeValue *left, *right;
  ViviCodeStatement *statement_right;

  *value = NULL;
  *statement = NULL;

  expected = next_parse_function (scanner, value, statement);
  if (expected != TOKEN_NONE)
    return expected;

  for (i = 0; tokens[i] != TOKEN_NONE; i++) {
    if (check_token (scanner, tokens[i])) {
      expected = next_parse_function (scanner, &right, &statement_right);
      if (expected != TOKEN_NONE) {
	g_object_unref (*value);
	*value = NULL;
	return FAIL (expected);
      }

      if (statement_right != NULL) {
	ViviCodeStatement *tmp;

	switch (pass) {
	  case PASS_LOGICAL_OR:
	    tmp = vivi_code_if_new (vivi_code_unary_new (*value, '!'));
	    vivi_code_if_set_if (VIVI_CODE_IF (tmp), statement_right);
	    g_object_unref (statement_right);
	    statement_right = tmp;
	    break;
	  case PASS_LOGICAL_AND:
	    tmp = vivi_code_if_new (*value);
	    vivi_code_if_set_if (VIVI_CODE_IF (tmp), statement_right);
	    g_object_unref (statement_right);
	    statement_right = tmp;
	    break;
	  case PASS_ALWAYS:
	    // nothing
	    break;
	  default:
	    g_assert_not_reached ();
	}

	*statement = vivi_compiler_combine_statements (2, *statement,
	    *statement_right);
      }

      left = VIVI_CODE_VALUE (*value);
      *value = vivi_code_binary_new_name (left, VIVI_CODE_VALUE (right),
	  vivi_compiler_scanner_token_name (tokens[i]));
      g_object_unref (left);
      g_object_unref (right);
    };
  }

  return TOKEN_NONE;
}

static int
parse_multiplicative_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_MULTIPLY,
    TOKEN_DIVIDE, TOKEN_REMAINDER, TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_ALWAYS, parse_unary_expression);
}

static int
parse_additive_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_ALWAYS, parse_multiplicative_expression);
}

static int
parse_shift_expression (ViviCompilerScanner *scanner, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT, TOKEN_SHIFT_RIGHT_UNSIGNED, TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_ALWAYS, parse_additive_expression);
}

static int
parse_relational_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN, /*TOKEN_LESS_THAN_OR_EQUAL,
    TOKEN_EQUAL_OR_GREATER_THAN, TOKEN_INSTANCEOF, TOKEN_IN,*/ TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_ALWAYS, parse_shift_expression);
}

static int
parse_equality_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_EQUAL,
    /*TOKEN_NOT_EQUAL,*/ TOKEN_STRICT_EQUAL, /*TOKEN_NOT_STRICT_EQUAL,*/
    TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_ALWAYS, parse_relational_expression);
}

static int
parse_bitwise_and_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_BITWISE_AND,
    TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_ALWAYS, parse_equality_expression);
}

static int
parse_bitwise_xor_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_BITWISE_XOR,
    TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_ALWAYS, parse_bitwise_and_expression);
}

static int
parse_bitwise_or_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_BITWISE_OR,
    TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_ALWAYS, parse_bitwise_xor_expression);
}

static int
parse_logical_and_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_LOGICAL_AND,
    TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_LOGICAL_AND, parse_bitwise_or_expression);
}

static int
parse_logical_or_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_LOGICAL_OR,
    TOKEN_NONE };

  return parse_operator_expression (scanner, value, statement, tokens,
      PASS_LOGICAL_OR, parse_logical_and_expression);
}

static int
parse_conditional_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  int expected;
  //ViviCodeStatement *if_statement, *else_statement;

  *value = NULL;

  expected = parse_logical_or_expression (scanner, value, statement);
  if (expected != TOKEN_NONE)
    return expected;

#if 0
  if (!check_token (scanner, TOKEN_QUESTION_MARK)) {
    *statement = vivi_code_value_statement_new (VIVI_CODE_VALUE (value));
    g_object_unref (value);
    return TOKEN_NONE;
  }

  expected = parse_assignment_expression (scanner, &if_statement);
  if (expected != TOKEN_NONE) {
    g_object_unref (value);
    return FAIL (expected);
  }

  if (!check_token (scanner, TOKEN_COLON)) {
    g_object_unref (value);
    g_object_unref (if_statement);
    return TOKEN_COLON;
  }

  expected = parse_assignment_expression (scanner, &else_statement);
  if (expected != TOKEN_NONE) {
    g_object_unref (value);
    g_object_unref (if_statement);
    return FAIL (expected);
  }

  *statement = vivi_code_if_new (value);
  vivi_code_if_set_if (VIVI_CODE_IF (*statement), if_statement);
  vivi_code_if_set_else (VIVI_CODE_IF (*statement), else_statement);

  g_object_unref (value);
  g_object_unref (if_statement);
  g_object_unref (else_statement);
#endif

  return TOKEN_NONE;
}

static int
parse_assignment_expression (ViviCompilerScanner *scanner,
    ViviCodeValue **value, ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *right;
  ViviCodeStatement *assignment, *statement_right;
  const char *operator;

  *value = NULL;
  *statement = NULL;

  expected = parse_conditional_expression (scanner, value, statement);
  if (expected != TOKEN_NONE)
    return expected;

  if (!VIVI_IS_CODE_GET (*value))
    return TOKEN_NONE;

  operator = NULL;

  vivi_compiler_scanner_peek_next_token (scanner);
  switch ((int)scanner->next_token) {
    case TOKEN_ASSIGN_MULTIPLY:
      if (operator == NULL) operator = "*";
    case TOKEN_ASSIGN_DIVIDE:
      if (operator == NULL) operator = "/";
    case TOKEN_ASSIGN_REMAINDER:
      if (operator == NULL) operator = "%";
    case TOKEN_ASSIGN_ADD:
      if (operator == NULL) operator = "+";
    case TOKEN_ASSIGN_MINUS:
      if (operator == NULL) operator = "-";
    case TOKEN_ASSIGN_SHIFT_LEFT:
      if (operator == NULL) operator = "<<";
    case TOKEN_ASSIGN_SHIFT_RIGHT:
      if (operator == NULL) operator = ">>";
    case TOKEN_ASSIGN_SHIFT_RIGHT_ZERO:
      if (operator == NULL) operator = ">>>";
    case TOKEN_ASSIGN_BITWISE_AND:
      if (operator == NULL) operator = "&";
    case TOKEN_ASSIGN_BITWISE_OR:
      if (operator == NULL) operator = "|";
    case TOKEN_ASSIGN_BITWISE_XOR:
      if (operator == NULL) operator = "^";
    case TOKEN_ASSIGN:
      vivi_compiler_scanner_get_next_token (scanner);
      expected = parse_assignment_expression (scanner, &right,
	  &statement_right);
      if (expected != TOKEN_NONE) {
	g_object_unref (*value);
	*value = NULL;
	return FAIL (expected);
      }

      if (operator != NULL) {
	assignment = vivi_compiler_assignment_new (*value,
	    vivi_code_binary_new_name (*value, right, operator));
      } else {
	assignment = vivi_compiler_assignment_new (*value, right);
      }
      g_object_unref (right);

      *statement = vivi_compiler_combine_statements (3, *statement,
	  statement_right, assignment);

      break;
    default:
      return TOKEN_NONE;
  }


  return TOKEN_NONE;
}

static int
parse_expression (ViviCompilerScanner *scanner, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeStatement *statement_one;
  int expected;

  *statement = NULL;
  statement_one = NULL;

  expected = parse_assignment_expression (scanner, value, &statement_one);
  if (expected != TOKEN_NONE)
    return expected;

  do {
    *statement =
      vivi_compiler_combine_statements (2, *statement, statement_one);

    if (!check_token (scanner, TOKEN_COMMA))
      break;

    statement_one = NULL;
    expected = parse_assignment_expression (scanner, value, &statement_one);
    if (expected != TOKEN_NONE && expected >= 0) {
      g_object_unref (*value);
      *value = NULL;
      g_object_unref (*statement);
      *statement = NULL;
      return expected;
    }
  } while (expected == TOKEN_NONE);

  return TOKEN_NONE;
}

// statement

static int
parse_statement (ViviCompilerScanner *scanner, ViviCodeStatement **statement);

static int
parse_if_statement (ViviCompilerScanner *scanner,
    ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *condition;
  ViviCodeStatement *pre_statement, *if_statement, *else_statement;

  *statement = NULL;

  if (!check_token (scanner, TOKEN_IF))
    return -TOKEN_IF;

  if (!check_token (scanner, TOKEN_PARENTHESIS_LEFT))
    return TOKEN_PARENTHESIS_LEFT;

  expected = parse_expression (scanner, &condition, &pre_statement);
  if (expected != TOKEN_NONE)
    return FAIL (expected);

  if (!check_token (scanner, TOKEN_PARENTHESIS_RIGHT)) {
    g_object_unref (condition);
    if (pre_statement != NULL) {
      g_object_unref (pre_statement);
      pre_statement = NULL;
    }
    return TOKEN_PARENTHESIS_RIGHT;
  }

  expected = parse_statement (scanner, &if_statement);
  if (expected != TOKEN_NONE) {
    g_object_unref (condition);
    if (pre_statement != NULL) {
      g_object_unref (pre_statement);
      pre_statement = NULL;
    }
    return FAIL (expected);
  }

  if (check_token (scanner, TOKEN_ELSE)) {
    expected = parse_statement (scanner, &else_statement);
    if (expected != TOKEN_NONE) {
      g_object_unref (condition);
      if (pre_statement != NULL) {
	g_object_unref (pre_statement);
	pre_statement = NULL;
      }
      g_object_unref (if_statement);
      return FAIL (expected);
    }
  } else {
    else_statement = NULL;
  }

  *statement = vivi_code_if_new (condition);
  g_object_unref (condition);

  vivi_code_if_set_if (VIVI_CODE_IF (*statement), if_statement);
  g_object_unref (if_statement);

  if (else_statement != NULL) {
    vivi_code_if_set_else (VIVI_CODE_IF (*statement), else_statement);
    g_object_unref (else_statement);
  }

  *statement = vivi_compiler_combine_statements (2, pre_statement, *statement);

  g_assert (*statement != NULL);

  return TOKEN_NONE;
}

static int
parse_expression_statement (ViviCompilerScanner *scanner,
    ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *value;

  *statement = NULL;

  vivi_compiler_scanner_peek_next_token (scanner);
  if (scanner->next_token == TOKEN_BRACE_LEFT ||
      scanner->next_token == TOKEN_FUNCTION)
    return -ERROR_TOKEN_EXPRESSION_STATEMENT;

  expected = parse_expression (scanner, &value, statement);
  if (expected != TOKEN_NONE)
    return expected;

  if (!check_token (scanner, TOKEN_SEMICOLON)) {
    g_object_unref (value);
    if (*statement != NULL) {
      g_object_unref (*statement);
      *statement = NULL;
    }
    return TOKEN_SEMICOLON;
  }

  *statement = vivi_compiler_combine_statements (2, *statement,
      vivi_code_value_statement_new (value));
  g_object_unref (value);

  return TOKEN_NONE;
}

static int
parse_empty_statement (ViviCompilerScanner *scanner,
    ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (scanner, TOKEN_SEMICOLON))
    return -TOKEN_SEMICOLON;

  *statement = vivi_compiler_empty_statement_new ();

  return TOKEN_NONE;
}

static int
parse_block (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  if (!check_token (scanner, TOKEN_BRACE_LEFT))
    return -TOKEN_BRACE_LEFT;

  vivi_compiler_scanner_peek_next_token (scanner);
  if (scanner->next_token != TOKEN_BRACE_RIGHT) {
    expected =
      parse_statement_list (scanner, parse_statement, statement, TOKEN_NONE);
    if (expected != TOKEN_NONE)
      return FAIL (expected);
  } else {
    *statement = vivi_code_block_new ();
  }

  if (!check_token (scanner, TOKEN_BRACE_RIGHT)) {
    g_object_unref (*statement);
    *statement = NULL;
    return TOKEN_BRACE_RIGHT;
  }

  return TOKEN_NONE;
}

static int
parse_variable_statement (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  if (!check_token (scanner, TOKEN_VAR))
    return -TOKEN_VAR;

  expected = parse_statement_list (scanner, parse_variable_declaration,
      statement, TOKEN_COMMA);
  if (expected != TOKEN_NONE)
    return FAIL (expected);

  if (!check_token (scanner, TOKEN_SEMICOLON)) {
    g_object_unref (*statement);
    *statement = NULL;
    return TOKEN_SEMICOLON;
  }

  return TOKEN_NONE;
}

static int
parse_statement (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  int i, expected;
  ParseStatementFunction functions[] = {
    parse_block,
    parse_variable_statement,
    parse_empty_statement,
    parse_expression_statement,
    parse_if_statement,
    //parse_iteration_statement,
    //parse_continue_statement,
    //parse_break_statement,
    //parse_return_statement,
    //parse_with_statement,
    //parse_labelled_statement,
    //parse_switch_statement,
    //parse_throw_statement,
    //parse_try_statement,
    NULL
  };

  *statement = NULL;

  for (i = 0; functions[i] != NULL; i++) {
    expected = functions[i] (scanner, statement);
    if (expected >= 0) {
      g_assert ((expected == TOKEN_NONE && *statement != NULL) ||
	  (expected != TOKEN_NONE && *statement == NULL));
      return expected;
    }
  }

  return -ERROR_TOKEN_STATEMENT;
}

// function

static int
parse_source_element (ViviCompilerScanner *scanner, ViviCodeStatement **statement);

static int
parse_function_declaration (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  ViviCodeValue *function, *identifier;
  ViviCodeValue **arguments;
  ViviCodeStatement *body;
  int expected;

  *statement = NULL;

  identifier = NULL;
  arguments = NULL;
  body = NULL;

  if (!check_token (scanner, TOKEN_FUNCTION))
    return -TOKEN_FUNCTION;

  expected = parse_identifier (scanner, &identifier);
  if (expected != TOKEN_NONE)
    return FAIL (expected);

  if (!check_token (scanner, TOKEN_PARENTHESIS_LEFT)) {
    g_object_unref (identifier);
    return TOKEN_PARENTHESIS_LEFT;
  }

  expected =
    parse_value_list (scanner, parse_identifier, &arguments, TOKEN_COMMA);
  if (expected != TOKEN_NONE && expected >= 0)
    return expected;

  if (!check_token (scanner, TOKEN_PARENTHESIS_RIGHT)) {
    g_object_unref (identifier);
    free_value_list (arguments);
    return TOKEN_PARENTHESIS_RIGHT;
  }

  if (!check_token (scanner, TOKEN_BRACE_LEFT)) {
    g_object_unref (identifier);
    free_value_list (arguments);
    return TOKEN_BRACE_LEFT;
  }

  expected = parse_statement_list (scanner, parse_source_element, &body,
      TOKEN_NONE);
  if (expected != TOKEN_NONE && expected >= 0) {
    g_object_unref (identifier);
    free_value_list (arguments);
    return expected;
  }

  if (!check_token (scanner, TOKEN_BRACE_RIGHT)) {
    g_object_unref (identifier);
    free_value_list (arguments);
    g_object_unref (body);
    return TOKEN_BRACE_RIGHT;
  }

  function = vivi_code_function_new ();
  vivi_code_function_set_body (VIVI_CODE_FUNCTION (function), body);
  *statement = vivi_compiler_assignment_new (VIVI_CODE_VALUE (identifier),
      VIVI_CODE_VALUE (function));

  g_object_unref (identifier);
  free_value_list (arguments);
  g_object_unref (body);

  return TOKEN_NONE;
}

// top

static int
parse_source_element (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  expected = parse_function_declaration (scanner, statement);
  if (expected < 0)
    expected = parse_statement (scanner, statement);

  return expected;
}

static int
parse_program (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  expected = parse_statement_list (scanner, parse_source_element, statement,
      TOKEN_NONE);
  if (expected != TOKEN_NONE)
    return FAIL (expected);

  if (!check_token (scanner, TOKEN_EOF)) {
    *statement = NULL;
    return FAIL (parse_statement (scanner, statement));
  }

  return TOKEN_NONE;
}

// parsing

static int
parse_statement_list (ViviCompilerScanner *scanner,
    ParseStatementFunction function, ViviCodeStatement **block,
    guint separator)
{
  ViviCodeStatement *statement;
  int expected;

  g_assert (scanner != NULL);
  g_assert (function != NULL);
  g_assert (block != NULL);

  *block = NULL;

  expected = function (scanner, &statement);
  if (expected != TOKEN_NONE)
    return expected;

  *block = vivi_code_block_new ();

  do {
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (*block), statement);
    g_object_unref (statement);

    if (separator != TOKEN_NONE && !check_token (scanner, separator))
      break;

    expected = function (scanner, &statement);
    if (expected != TOKEN_NONE && expected >= 0) {
      g_object_unref (*block);
      *block = NULL;
      return expected;
    }
  } while (expected == TOKEN_NONE);

  return TOKEN_NONE;
}

static int
parse_value_list (ViviCompilerScanner *scanner, ParseValueFunction function,
    ViviCodeValue ***list, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;
  int expected;

  g_assert (scanner != NULL);
  g_assert (function != NULL);
  g_assert (list != NULL);

  *list = NULL;

  expected = function (scanner, &value);
  if (expected != TOKEN_NONE)
    return expected;

  array = g_ptr_array_new ();

  do {
    g_ptr_array_add (array, value);

    if (separator != TOKEN_NONE && !check_token (scanner, separator))
      break;

    expected = function (scanner, &value);
    if (expected != TOKEN_NONE && expected >= 0)
      return expected;
  } while (expected == TOKEN_NONE);
  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);

  return TOKEN_NONE;
}

// public

ViviCodeStatement *
vivi_compile_file (FILE *file, const char *input_name)
{
  ViviCompilerScanner *scanner;
  ViviCodeStatement *statement;
  int expected;

  g_return_val_if_fail (file != NULL, NULL);

  scanner = vivi_compiler_scanner_new (file);

  expected = parse_program (scanner, &statement);
  g_assert ((expected == TOKEN_NONE && VIVI_IS_CODE_STATEMENT (statement)) ||
	(expected != TOKEN_NONE && statement == NULL));
  g_assert (expected >= 0);

  if (expected != TOKEN_NONE) {
    vivi_compiler_scanner_get_next_token (scanner);
    if (expected < TOKEN_LAST) {
      vivi_compiler_scanner_unexp_token (scanner, expected);
    } else {
      guint i;
      const char *name;

      name = NULL;
      for (i = 0; error_names[i].token != TOKEN_LAST; i++) {
	if (error_names[i].token == expected) {
	  name = error_names[i].name;
	  break;
	}
      }

      g_assert (name != NULL);

      vivi_compiler_scanner_unexp_token_string (scanner, name);
    }
  }

  g_object_unref (scanner);

  return statement;
}
