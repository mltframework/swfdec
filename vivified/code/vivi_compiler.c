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

#include "vivi_code_text_printer.h"

enum {
  // keywords
  TOKEN_BREAK = G_TOKEN_LAST + 1,
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
  // values
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_NULL,
  // misc
  TOKEN_PLUSPLUS,
  TOKEN_MINUSMINUS,
  // errors
  ERROR_TOKEN_BOOLEAN,
  ERROR_TOKEN_LITERAL,
  ERROR_TOKEN_PROPERTY_NAME,
  ERROR_TOKEN_PRIMARY_EXPRESSION,
  ERROR_TOKEN_EXPRESSION_STATEMENT,
  ERROR_TOKEN_STATEMENT
};

typedef struct {
  guint		token;
  const char *	symbol;
} TokenDescription;

static const TokenDescription custom_tokens[] = {
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
  { TOKEN_VOID, "while" },
  { TOKEN_WITH, "with" },
  // reserved keywords
  { TOKEN_FUTURE, "abstract" },
  { TOKEN_FUTURE, "boolean" },
  { TOKEN_FUTURE, "byte" },
  { TOKEN_FUTURE, "char" },
  { TOKEN_FUTURE, "class" },
  { TOKEN_FUTURE, "const" },
  { TOKEN_FUTURE, "debugger" },
  { TOKEN_FUTURE, "double" },
  { TOKEN_FUTURE, "enum" },
  { TOKEN_FUTURE, "export" },
  { TOKEN_FUTURE, "extends" },
  { TOKEN_FUTURE, "final" },
  { TOKEN_FUTURE, "float" },
  { TOKEN_FUTURE, "goto" },
  { TOKEN_FUTURE, "implements" },
  { TOKEN_FUTURE, "import" },
  { TOKEN_FUTURE, "int" },
  { TOKEN_FUTURE, "interface" },
  { TOKEN_FUTURE, "long" },
  { TOKEN_FUTURE, "native" },
  { TOKEN_FUTURE, "package" },
  { TOKEN_FUTURE, "private" },
  { TOKEN_FUTURE, "protected" },
  { TOKEN_FUTURE, "public" },
  { TOKEN_FUTURE, "short" },
  { TOKEN_FUTURE, "static" },
  { TOKEN_FUTURE, "super" },
  { TOKEN_FUTURE, "synchronized" },
  { TOKEN_FUTURE, "throws" },
  { TOKEN_FUTURE, "transient" },
  { TOKEN_FUTURE, "volatile" },
  // values
  { TOKEN_TRUE, "true" },
  { TOKEN_FALSE, "false" },
  { TOKEN_NULL, "null" },
  // misc
  { TOKEN_PLUSPLUS, "++" },
  { TOKEN_MINUSMINUS, "--" },
  { G_TOKEN_NONE, NULL }
};

static const TokenDescription error_tokens[] = {
  // errors
  { ERROR_TOKEN_BOOLEAN, "boolean" },
  { ERROR_TOKEN_LITERAL, "literal" },
  { ERROR_TOKEN_PROPERTY_NAME, "property name" },
  { ERROR_TOKEN_PRIMARY_EXPRESSION, "primary expression" },
  { ERROR_TOKEN_EXPRESSION_STATEMENT, "expression statement" },
  { ERROR_TOKEN_STATEMENT, "statement" },
  { G_TOKEN_NONE, NULL }
};

#define FAIL(x) ((unsigned) ((x) < 0 ? -x : x))

typedef int (*ParseStatementFunction) (GScanner *scanner, ViviCodeStatement **statement);
typedef int (*ParseValueFunction) (GScanner *scanner, ViviCodeValue **value);

static int
parse_statement_list (GScanner *scanner, ParseStatementFunction function,
    ViviCodeStatement **statement, guint separator);
static int
parse_value_list (GScanner *scanner, ParseValueFunction function,
    ViviCodeValue ***list, guint separator);

// helpers

static gboolean
check_token (GScanner *scanner, guint token)
{
  g_scanner_peek_next_token (scanner);
  if (scanner->next_token != token)
    return FALSE;
  g_scanner_get_next_token (scanner);
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

// values

static int
parse_null_literal (GScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, TOKEN_NULL))
    return -TOKEN_NULL;

  *value = vivi_code_constant_new_null ();
  return G_TOKEN_NONE;
}

static int
parse_boolean_literal (GScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (check_token (scanner, TOKEN_TRUE)) {
    *value = vivi_code_constant_new_boolean (TRUE);
    return G_TOKEN_NONE;
  } else if (check_token (scanner, TOKEN_FALSE)) {
    *value = vivi_code_constant_new_boolean (FALSE);
    return G_TOKEN_NONE;
  } else {
    return -ERROR_TOKEN_BOOLEAN;
  }
}

static int
parse_numeric_literal (GScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, G_TOKEN_FLOAT))
    return -G_TOKEN_FLOAT;

  *value = vivi_code_constant_new_number (scanner->value.v_float);
  return G_TOKEN_NONE;
}

static int
parse_string_literal (GScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, G_TOKEN_STRING))
    return -G_TOKEN_STRING;

  *value = vivi_code_constant_new_string (scanner->value.v_string);
  return G_TOKEN_NONE;
}

static int
parse_literal (GScanner *scanner, ViviCodeValue **value)
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
parse_identifier (GScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, G_TOKEN_IDENTIFIER))
    return -G_TOKEN_IDENTIFIER;

  *value = vivi_code_get_new_name (scanner->value.v_identifier);

  return G_TOKEN_NONE;
}

static int
parse_property_name (GScanner *scanner, ViviCodeValue **value)
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
parse_operator_expression (GScanner *scanner, ViviCodeValue **value);

static int
parse_object_literal (GScanner *scanner, ViviCodeValue **value)
{
  int expected;

  *value = NULL;

  if (!check_token (scanner, '{'))
    return -'{';

  *value = vivi_code_init_object_new ();

  if (!check_token (scanner, '}')) {
    do {
      ViviCodeValue *property, *initializer;

      expected = parse_property_name (scanner, &property);
      if (expected != G_TOKEN_NONE) {
	g_object_unref (*value);
	*value = NULL;
	return FAIL (expected);
      }

      if (!check_token (scanner, ':')) {
	g_object_unref (*value);
	*value = NULL;
	return ':';
      }

      // FIXME: assignment expression
      expected = parse_operator_expression (scanner, &initializer);
      if (expected != G_TOKEN_NONE) {
	g_object_unref (*value);
	*value = NULL;
	return FAIL (expected);
      }

      vivi_code_init_object_add_variable (VIVI_CODE_INIT_OBJECT (*value),
	  property, initializer);
    } while (check_token (scanner, ','));
  }

  if (!check_token (scanner, '}')) {
    g_object_unref (*value);
    *value = NULL;
    return '}';
  }

  return G_TOKEN_NONE;
}

// misc

static int
parse_variable_declaration (GScanner *scanner, ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *identifier;
  ViviCodeValue *value;

  *statement = NULL;

  expected = parse_identifier (scanner, &identifier);
  if (expected != G_TOKEN_NONE)
    return expected;

  if (check_token (scanner, '=')) {
    // FIXME: assignment expression
    expected = parse_operator_expression (scanner, &value);
    if (expected != G_TOKEN_NONE) {
      g_object_unref (identifier);
      return FAIL (expected);
    }
  } else {
    value = vivi_code_constant_new_undefined ();
  }

  *statement = vivi_code_assignment_new (NULL, identifier, value);
  vivi_code_assignment_set_local (VIVI_CODE_ASSIGNMENT (*statement), TRUE);

  return G_TOKEN_NONE;
}

// expression

static int
parse_primary_expression (GScanner *scanner, ViviCodeValue **value)
{
  int i, expected;
  ParseValueFunction functions[] = {
    parse_identifier,
    parse_literal,
    //parse_array_literal,
    parse_object_literal,
    NULL
  };

  *value = NULL;

  if (check_token (scanner, TOKEN_THIS)) {
    *value = vivi_code_get_new_name ("this");
    return G_TOKEN_NONE;
  }

  if (check_token (scanner, '(')) {
    // FIXME: assignment expression
    expected = parse_operator_expression (scanner, value);
    if (expected != G_TOKEN_NONE)
      return FAIL (expected);
    if (!check_token (scanner, ')')) {
      g_object_unref (*value);
      *value = NULL;
      return ')';
    }
    return G_TOKEN_NONE;
  }

  for (i = 0; functions[i] != NULL; i++) {
    expected = functions[i] (scanner, value);
    if (expected >= 0)
      return expected;
  }

  return -ERROR_TOKEN_PRIMARY_EXPRESSION;
}

static int
parse_member_expression (GScanner *scanner, ViviCodeValue **value)
{
  int expected;
  ViviCodeValue *member;

  // TODO: new MemberExpression Arguments

  expected = parse_primary_expression (scanner, value);
  //if (expected == STATUS_CANCEL)
  //  expected = parse_function_expression (scanner, value);

  if (expected != G_TOKEN_NONE)
    return expected;

  do {
    ViviCodeValue *tmp;

    if (check_token (scanner, '[')) {
      // FIXME: expression
      expected = parse_operator_expression (scanner, &member);
      if (expected != G_TOKEN_NONE)
	return FAIL (expected);
      if (!check_token (scanner, ']'))
	return ']';
    } else if (check_token (scanner, '.')) {
      expected = parse_identifier (scanner, &member);
      if (expected != G_TOKEN_NONE)
	return FAIL (expected);
    } else {
      return G_TOKEN_NONE;
    }

    tmp = *value;
    *value = vivi_code_get_new (tmp, VIVI_CODE_VALUE (member));
    g_object_unref (tmp);
    g_object_unref (member);
  } while (TRUE);

  g_assert_not_reached ();
}

static int
parse_new_expression (GScanner *scanner, ViviCodeValue **value)
{
  int expected;

  *value = NULL;

  if (check_token (scanner, TOKEN_NEW)) {
    expected = parse_new_expression (scanner, value);
    if (expected != G_TOKEN_NONE)
      return FAIL (expected);
    if (!VIVI_IS_CODE_FUNCTION_CALL (*value)) {
      ViviCodeValue *tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_code_function_call_new (NULL, tmp);
      g_object_unref (tmp);
    }
    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (*value),
	TRUE);
    return G_TOKEN_NONE;
  } else {
    return parse_member_expression (scanner, value);
  }
}

static int
parse_left_hand_side_expression (GScanner *scanner, ViviCodeValue **value)
{
  int expected;

  *value = NULL;

  expected = parse_new_expression (scanner, value);
  //if (expected == STATUS_CANCEL)
  //  expected = parse_call_expression (scanner, value);

  return expected;
}

static int
parse_postfix_expression (GScanner *scanner, ViviCodeValue **value)
{
  int expected;
  ViviCodeValue *tmp, *one;
  const char *operator;

  expected = parse_left_hand_side_expression (scanner, value);
  if (expected != G_TOKEN_NONE)
    return expected;

  // FIXME: Don't allow new line here

  if (check_token (scanner, TOKEN_PLUSPLUS)) {
    operator = "+";
  } else if (check_token (scanner, TOKEN_MINUSMINUS)) {
    operator = "-";
  } else {
    return G_TOKEN_NONE;
  }

  one = vivi_code_constant_new_number (1);

  // FIXME: Side effect!
  tmp = *value;
  *value = vivi_code_binary_new_name (tmp, one, operator);
  g_object_unref (tmp);

  g_object_unref (one);

  return G_TOKEN_NONE;
}

static int
parse_unary_expression (GScanner *scanner, ViviCodeValue **value)
{
  ViviCodeValue *tmp;

  *value = NULL;

  if (check_token (scanner, '!')) {
    int expected = parse_unary_expression (scanner, value);
    if (expected != G_TOKEN_NONE)
      return FAIL (expected);
    tmp = VIVI_CODE_VALUE (*value);
    *value = vivi_code_unary_new (tmp, '!');
    g_object_unref (tmp);
    return G_TOKEN_NONE;
  } else {
    return parse_postfix_expression (scanner, value);
  }
}

static int
parse_operator_expression (GScanner *scanner, ViviCodeValue **value)
{
  int expected;
  ViviCodeValue *left;
  ViviCodeValue *right;

  *value = NULL;

  expected = parse_unary_expression (scanner, value);
  if (expected != G_TOKEN_NONE)
    return expected;

  while (check_token (scanner, '+')) {
    expected = parse_unary_expression (scanner, &right);
    if (expected != G_TOKEN_NONE) {
      g_object_unref (*value);
      *value = NULL;
      return FAIL (expected);
    }

    left = VIVI_CODE_VALUE (*value);
    *value = vivi_code_binary_new_name (left, VIVI_CODE_VALUE (right), "+");
    g_object_unref (left);
    g_object_unref (right);
  };

  return G_TOKEN_NONE;
}

static int
parse_assignment_expression (GScanner *scanner, ViviCodeStatement **statement);

static int
parse_conditional_expression (GScanner *scanner, ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *value;
  ViviCodeStatement *if_statement, *else_statement;

  *statement = NULL;

  expected = parse_operator_expression (scanner, &value);
  if (expected != G_TOKEN_NONE)
    return expected;

  if (!check_token (scanner, '?')) {
    *statement = vivi_code_value_statement_new (VIVI_CODE_VALUE (value));
    g_object_unref (value);
    return G_TOKEN_NONE;
  }

  expected = parse_assignment_expression (scanner, &if_statement);
  if (expected != G_TOKEN_NONE) {
    g_object_unref (value);
    return FAIL (expected);
  }

  if (!check_token (scanner, ':')) {
    g_object_unref (value);
    g_object_unref (if_statement);
    return ':';
  }

  expected = parse_assignment_expression (scanner, &else_statement);
  if (expected != G_TOKEN_NONE) {
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

  return G_TOKEN_NONE;
}

static int
parse_assignment_expression (GScanner *scanner, ViviCodeStatement **statement)
{
  *statement = NULL;

  // TODO

  return parse_conditional_expression (scanner, statement);
}

static int
parse_expression (GScanner *scanner, ViviCodeStatement **statement)
{
  *statement = NULL;

  return parse_statement_list (scanner, parse_assignment_expression, statement,
      ',');
}

// statement

static int
parse_expression_statement (GScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  g_scanner_peek_next_token (scanner);
  if (scanner->next_token == '{' || scanner->next_token == TOKEN_FUNCTION)
    return -ERROR_TOKEN_EXPRESSION_STATEMENT;

  expected = parse_expression (scanner, statement);
  if (expected != G_TOKEN_NONE)
    return expected;

  if (!check_token (scanner, ';')) {
    g_object_unref (*statement);
    *statement = NULL;
    return ';';
  }

  return G_TOKEN_NONE;
}

static int
parse_empty_statement (GScanner *scanner, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (scanner, ';'))
    return -';';

  *statement = vivi_compiler_empty_statement_new ();

  return G_TOKEN_NONE;
}

static int
parse_statement (GScanner *scanner, ViviCodeStatement **statement);

static int
parse_block (GScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  if (!check_token (scanner, '{'))
    return -'{';

  g_scanner_peek_next_token (scanner);
  if (scanner->next_token != '}') {
    expected =
      parse_statement_list (scanner, parse_statement, statement, G_TOKEN_NONE);
    if (expected != G_TOKEN_NONE)
      return FAIL (expected);
  } else {
    *statement = vivi_code_block_new ();
  }

  if (!check_token (scanner, '}')) {
    g_object_unref (*statement);
    *statement = NULL;
    return '}';
  }

  return G_TOKEN_NONE;
}

static int
parse_variable_statement (GScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  if (!check_token (scanner, TOKEN_VAR))
    return -TOKEN_VAR;

  expected =
    parse_statement_list (scanner, parse_variable_declaration, statement, ',');
  if (expected != G_TOKEN_NONE)
    return FAIL (expected);

  if (!check_token (scanner, ';')) {
    g_object_unref (*statement);
    *statement = NULL;
    return ';';
  }

  return G_TOKEN_NONE;
}

static int
parse_statement (GScanner *scanner, ViviCodeStatement **statement)
{
  int i, expected;
  ParseStatementFunction functions[] = {
    parse_block,
    parse_variable_statement,
    parse_empty_statement,
    parse_expression_statement,
    //parse_if_statement,
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
    if (expected >= 0)
      return expected;
  }

  return -ERROR_TOKEN_STATEMENT;
}

// function

static int
parse_source_element (GScanner *scanner, ViviCodeStatement **statement);

static int
parse_function_declaration (GScanner *scanner, ViviCodeStatement **statement)
{
  //ViviCodeStatement *function;
  ViviCodeValue *identifier;
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
  if (expected != G_TOKEN_NONE)
    return FAIL (expected);

  if (!check_token (scanner, '(')) {
    g_object_unref (identifier);
    return '(';
  }

  expected = parse_value_list (scanner, parse_identifier, &arguments, ',');
  if (expected != G_TOKEN_NONE && expected >= 0)
    return expected;

  if (!check_token (scanner, ')')) {
    g_object_unref (identifier);
    free_value_list (arguments);
    return ')';
  }

  if (!check_token (scanner, '{')) {
    g_object_unref (identifier);
    free_value_list (arguments);
    return '{';
  }

  expected = parse_statement_list (scanner, parse_source_element, &body,
      G_TOKEN_NONE);
  if (expected != G_TOKEN_NONE && expected >= 0) {
    g_object_unref (identifier);
    free_value_list (arguments);
    return expected;
  }

  if (!check_token (scanner, '}')) {
    g_object_unref (identifier);
    free_value_list (arguments);
    g_object_unref (body);
    return '}';
  }

  /*function = vivi_code_function_new (arguments, body);
  *statement = vivi_code_assignment_new (NULL, VIVI_CODE_VALUE (identifier),
      VIVI_CODE_VALUE (function));*/
  *statement = vivi_compiler_empty_statement_new ();

  g_object_unref (identifier);
  free_value_list (arguments);
  g_object_unref (body);

  return G_TOKEN_NONE;
}

// top

static int
parse_source_element (GScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  expected = parse_function_declaration (scanner, statement);
  if (expected < 0)
    expected = parse_statement (scanner, statement);

  return expected;
}

static int
parse_program (GScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  expected = parse_statement_list (scanner, parse_source_element, statement,
      G_TOKEN_NONE);
  if (expected != G_TOKEN_NONE)
    return FAIL (expected);

  if (!check_token (scanner, G_TOKEN_EOF)) {
    *statement = NULL;
    return FAIL (parse_statement (scanner, statement));
  }

  return G_TOKEN_NONE;
}

// parsing

static int
parse_statement_list (GScanner *scanner, ParseStatementFunction function,
    ViviCodeStatement **block, guint separator)
{
  ViviCodeStatement *statement;
  int expected;

  g_assert (scanner != NULL);
  g_assert (function != NULL);
  g_assert (block != NULL);

  *block = NULL;

  expected = function (scanner, &statement);
  if (expected != G_TOKEN_NONE)
    return expected;

  *block = vivi_code_block_new ();

  do {
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (*block), statement);
    g_object_unref (statement);

    if (separator != G_TOKEN_NONE && !check_token (scanner, separator))
      break;

    expected = function (scanner, &statement);
    if (expected != G_TOKEN_NONE && expected >= 0) {
      g_object_unref (*block);
      *block = NULL;
      return expected;
    }
  } while (expected == G_TOKEN_NONE);

  return G_TOKEN_NONE;
}

static int
parse_value_list (GScanner *scanner, ParseValueFunction function,
    ViviCodeValue ***list, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;
  int expected;

  g_assert (scanner != NULL);
  g_assert (function != NULL);
  g_assert (list != NULL);

  expected = function (scanner, &value);
  if (expected != G_TOKEN_NONE)
    return expected;

  array = g_ptr_array_new ();

  do {
    g_ptr_array_add (array, value);

    if (separator != G_TOKEN_NONE && !check_token (scanner, separator))
      break;

    expected = function (scanner, &value);
    if (expected != G_TOKEN_NONE && expected >= 0)
      return expected;
  } while (expected == G_TOKEN_NONE);
  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);

  return G_TOKEN_NONE;
}

// public

ViviCodeStatement *
vivi_compile_text (const char *text, gsize len, const char *input_name)
{
  GScanner *scanner;
  ViviCodeStatement *statement;
  int expected, i;

  g_return_val_if_fail (text != NULL, NULL);

  scanner = g_scanner_new (NULL);

  scanner->config->numbers_2_int = TRUE;
  scanner->config->int_2_float = TRUE;
  scanner->config->symbol_2_token = TRUE;
  // FIXME: Should allow other Unicode characters
  scanner->config->cset_identifier_first =
    g_strdup (G_CSET_A_2_Z G_CSET_a_2_z G_CSET_LATINS G_CSET_LATINC "_$");
  scanner->config->cset_identifier_nth = g_strdup (G_CSET_A_2_Z G_CSET_a_2_z
      G_CSET_LATINS G_CSET_LATINC "_$" G_CSET_DIGITS);
  scanner->config->scan_identifier_1char = TRUE;

  g_scanner_set_scope (scanner, 0);
  for (i = 0; custom_tokens[i].token != G_TOKEN_NONE; i++) {
    g_scanner_scope_add_symbol (scanner, 0, custom_tokens[i].symbol,
	GINT_TO_POINTER (custom_tokens[i].token));
  }

  scanner->input_name = input_name;
  g_scanner_input_text (scanner, text, len);

  expected = parse_program (scanner, &statement);
  g_assert ((expected == G_TOKEN_NONE && VIVI_IS_CODE_STATEMENT (statement)) ||
	(expected != G_TOKEN_NONE && statement == NULL));
  if (expected != G_TOKEN_NONE) {
    char *name = NULL;

    for (i = 0; custom_tokens[i].token != G_TOKEN_NONE; i++) {
      if (custom_tokens[i].token == FAIL (expected)) {
	name = g_ascii_strup (custom_tokens[i].symbol, -1);
	break;
      }
    }
    if (custom_tokens[i].token == G_TOKEN_NONE) {
      for (i = 0; error_tokens[i].token != G_TOKEN_NONE; i++) {
	if (error_tokens[i].token == FAIL (expected)) {
	  name = g_ascii_strup (error_tokens[i].symbol, -1);
	  break;
	}
      }
    }

    g_scanner_get_next_token (scanner);
    g_scanner_unexp_token (scanner, FAIL (expected), NULL, name, NULL, NULL,
	TRUE);
  }

  g_scanner_destroy (scanner);

  return statement;
}
