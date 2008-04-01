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

#include "vivi_code_text_printer.h"

#if 0
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
#endif

enum {
  ERROR_TOKEN_LITERAL = TOKEN_LAST + 1,
  ERROR_TOKEN_PROPERTY_NAME,
  ERROR_TOKEN_PRIMARY_EXPRESSION,
  ERROR_TOKEN_EXPRESSION_STATEMENT,
  ERROR_TOKEN_STATEMENT
};

#define FAIL(x) ((x) < 0 ? -x : x)

typedef int (*ParseStatementFunction) (ViviCompilerScanner *scanner, ViviCodeStatement **statement);
typedef int (*ParseValueFunction) (ViviCompilerScanner *scanner, ViviCodeValue **value);

static int
parse_statement_list (ViviCompilerScanner *scanner, ParseStatementFunction function, ViviCodeStatement **statement, guint separator);
static int
parse_value_list (ViviCompilerScanner *scanner, ParseValueFunction function, ViviCodeValue ***list, guint separator);

// helpers

static gboolean
check_token (ViviCompilerScanner *scanner, guint token)
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

  if (!check_token (scanner, G_TOKEN_STRING))
    return -G_TOKEN_STRING;

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

  if (!check_token (scanner, G_TOKEN_IDENTIFIER))
    return -G_TOKEN_IDENTIFIER;

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
parse_operator_expression (ViviCompilerScanner *scanner, ViviCodeValue **value);

static int
parse_object_literal (ViviCompilerScanner *scanner, ViviCodeValue **value)
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
      if (expected != TOKEN_NONE) {
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
      if (expected != TOKEN_NONE) {
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

  return TOKEN_NONE;
}

// misc

static int
parse_variable_declaration (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *identifier;
  ViviCodeValue *value;

  *statement = NULL;

  expected = parse_identifier (scanner, &identifier);
  if (expected != TOKEN_NONE)
    return expected;

  if (check_token (scanner, '=')) {
    // FIXME: assignment expression
    expected = parse_operator_expression (scanner, &value);
    if (expected != TOKEN_NONE) {
      g_object_unref (identifier);
      return FAIL (expected);
    }
  } else {
    value = vivi_code_constant_new_undefined ();
  }

  *statement = vivi_code_assignment_new (NULL, identifier, value);
  vivi_code_assignment_set_local (VIVI_CODE_ASSIGNMENT (*statement), TRUE);

  return TOKEN_NONE;
}

// expression

static int
parse_primary_expression (ViviCompilerScanner *scanner, ViviCodeValue **value)
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
    return TOKEN_NONE;
  }

  if (check_token (scanner, '(')) {
    // FIXME: assignment expression
    expected = parse_operator_expression (scanner, value);
    if (expected != TOKEN_NONE)
      return FAIL (expected);
    if (!check_token (scanner, ')')) {
      g_object_unref (*value);
      *value = NULL;
      return ')';
    }
    return TOKEN_NONE;
  }

  for (i = 0; functions[i] != NULL; i++) {
    expected = functions[i] (scanner, value);
    if (expected >= 0)
      return expected;
  }

  return -ERROR_TOKEN_PRIMARY_EXPRESSION;
}

static int
parse_member_expression (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  int expected;
  ViviCodeValue *member;

  // TODO: new MemberExpression Arguments

  expected = parse_primary_expression (scanner, value);
  //if (expected == STATUS_CANCEL)
  //  expected = parse_function_expression (scanner, value);

  if (expected != TOKEN_NONE)
    return expected;

  do {
    ViviCodeValue *tmp;

    if (check_token (scanner, '[')) {
      // FIXME: expression
      expected = parse_operator_expression (scanner, &member);
      if (expected != TOKEN_NONE)
	return FAIL (expected);
      if (!check_token (scanner, ']'))
	return ']';
    } else if (check_token (scanner, '.')) {
      expected = parse_identifier (scanner, &member);
      if (expected != TOKEN_NONE)
	return FAIL (expected);
    } else {
      return TOKEN_NONE;
    }

    tmp = *value;
    *value = vivi_code_get_new (tmp, VIVI_CODE_VALUE (member));
    g_object_unref (tmp);
    g_object_unref (member);
  } while (TRUE);

  g_assert_not_reached ();
}

static int
parse_new_expression (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  int expected;

  *value = NULL;

  if (check_token (scanner, TOKEN_NEW)) {
    expected = parse_new_expression (scanner, value);
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
    return parse_member_expression (scanner, value);
  }
}

static int
parse_left_hand_side_expression (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  int expected;

  *value = NULL;

  expected = parse_new_expression (scanner, value);
  //if (expected == STATUS_CANCEL)
  //  expected = parse_call_expression (scanner, value);

  return expected;
}

static int
parse_postfix_expression (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  int expected;
  ViviCodeValue *tmp, *one;
  const char *operator;

  expected = parse_left_hand_side_expression (scanner, value);
  if (expected != TOKEN_NONE)
    return expected;

  // FIXME: Don't allow new line here

  if (check_token (scanner, TOKEN_PLUSPLUS)) {
    operator = "+";
  } else if (check_token (scanner, TOKEN_MINUSMINUS)) {
    operator = "-";
  } else {
    return TOKEN_NONE;
  }

  one = vivi_code_constant_new_number (1);

  // FIXME: Side effect!
  tmp = *value;
  *value = vivi_code_binary_new_name (tmp, one, operator);
  g_object_unref (tmp);

  g_object_unref (one);

  return TOKEN_NONE;
}

static int
parse_unary_expression (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  ViviCodeValue *tmp;

  *value = NULL;

  if (check_token (scanner, '!')) {
    int expected = parse_unary_expression (scanner, value);
    if (expected != TOKEN_NONE)
      return FAIL (expected);
    tmp = VIVI_CODE_VALUE (*value);
    *value = vivi_code_unary_new (tmp, '!');
    g_object_unref (tmp);
    return TOKEN_NONE;
  } else {
    return parse_postfix_expression (scanner, value);
  }
}

static int
parse_operator_expression (ViviCompilerScanner *scanner, ViviCodeValue **value)
{
  int expected;
  ViviCodeValue *left;
  ViviCodeValue *right;

  *value = NULL;

  expected = parse_unary_expression (scanner, value);
  if (expected != TOKEN_NONE)
    return expected;

  while (check_token (scanner, '+')) {
    expected = parse_unary_expression (scanner, &right);
    if (expected != TOKEN_NONE) {
      g_object_unref (*value);
      *value = NULL;
      return FAIL (expected);
    }

    left = VIVI_CODE_VALUE (*value);
    *value = vivi_code_binary_new_name (left, VIVI_CODE_VALUE (right), "+");
    g_object_unref (left);
    g_object_unref (right);
  };

  return TOKEN_NONE;
}

static int
parse_assignment_expression (ViviCompilerScanner *scanner, ViviCodeStatement **statement);

static int
parse_conditional_expression (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  int expected;
  ViviCodeValue *value;
  ViviCodeStatement *if_statement, *else_statement;

  *statement = NULL;

  expected = parse_operator_expression (scanner, &value);
  if (expected != TOKEN_NONE)
    return expected;

  if (!check_token (scanner, '?')) {
    *statement = vivi_code_value_statement_new (VIVI_CODE_VALUE (value));
    g_object_unref (value);
    return TOKEN_NONE;
  }

  expected = parse_assignment_expression (scanner, &if_statement);
  if (expected != TOKEN_NONE) {
    g_object_unref (value);
    return FAIL (expected);
  }

  if (!check_token (scanner, ':')) {
    g_object_unref (value);
    g_object_unref (if_statement);
    return ':';
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

  return TOKEN_NONE;
}

static int
parse_assignment_expression (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  *statement = NULL;

  // TODO

  return parse_conditional_expression (scanner, statement);
}

static int
parse_expression (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  *statement = NULL;

  return parse_statement_list (scanner, parse_assignment_expression, statement,
      ',');
}

// statement

static int
parse_expression_statement (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  vivi_compiler_scanner_peek_next_token (scanner);
  if (scanner->next_token == '{' || scanner->next_token == TOKEN_FUNCTION)
    return -ERROR_TOKEN_EXPRESSION_STATEMENT;

  expected = parse_expression (scanner, statement);
  if (expected != TOKEN_NONE)
    return expected;

  if (!check_token (scanner, ';')) {
    g_object_unref (*statement);
    *statement = NULL;
    return ';';
  }

  return TOKEN_NONE;
}

static int
parse_empty_statement (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (scanner, ';'))
    return -';';

  *statement = vivi_compiler_empty_statement_new ();

  return TOKEN_NONE;
}

static int
parse_statement (ViviCompilerScanner *scanner, ViviCodeStatement **statement);

static int
parse_block (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
{
  int expected;

  *statement = NULL;

  if (!check_token (scanner, '{'))
    return -'{';

  vivi_compiler_scanner_peek_next_token (scanner);
  if (scanner->next_token != '}') {
    expected =
      parse_statement_list (scanner, parse_statement, statement, TOKEN_NONE);
    if (expected != TOKEN_NONE)
      return FAIL (expected);
  } else {
    *statement = vivi_code_block_new ();
  }

  if (!check_token (scanner, '}')) {
    g_object_unref (*statement);
    *statement = NULL;
    return '}';
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

  expected =
    parse_statement_list (scanner, parse_variable_declaration, statement, ',');
  if (expected != TOKEN_NONE)
    return FAIL (expected);

  if (!check_token (scanner, ';')) {
    g_object_unref (*statement);
    *statement = NULL;
    return ';';
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
parse_source_element (ViviCompilerScanner *scanner, ViviCodeStatement **statement);

static int
parse_function_declaration (ViviCompilerScanner *scanner, ViviCodeStatement **statement)
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
  if (expected != TOKEN_NONE)
    return FAIL (expected);

  if (!check_token (scanner, '(')) {
    g_object_unref (identifier);
    return '(';
  }

  expected = parse_value_list (scanner, parse_identifier, &arguments, ',');
  if (expected != TOKEN_NONE && expected >= 0)
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
      TOKEN_NONE);
  if (expected != TOKEN_NONE && expected >= 0) {
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

  if (!check_token (scanner, G_TOKEN_EOF)) {
    *statement = NULL;
    return FAIL (parse_statement (scanner, statement));
  }

  return TOKEN_NONE;
}

// parsing

static int
parse_statement_list (ViviCompilerScanner *scanner, ParseStatementFunction function,
    ViviCodeStatement **block, guint separator)
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

  g_object_unref (scanner);

  return statement;
}
