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

typedef enum {
  STATUS_FAIL = -1,
  STATUS_OK = 0,
  STATUS_CANCEL = 1
} ParseStatus;

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
  TOKEN_MINUSMINUS
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

typedef ParseStatus (*ParseStatementFunction) (GScanner *scanner, ViviCodeStatement **statement);
typedef ParseStatus (*ParseValueFunction) (GScanner *scanner, ViviCodeValue **value);

static ParseStatus
parse_statement_list (GScanner *scanner, ParseStatementFunction function,
    ViviCodeStatement ***list, guint separator);
static ParseStatus
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
free_statement_list (ViviCodeStatement **list)
{
  int i;

  for (i = 0; list[i] != NULL; i++) {
    g_object_unref (list[i]);
  }
  g_free (list);
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
create_block (ViviCodeStatement **list)
{
  ViviCodeBlock *block;
  int i;

  block = VIVI_CODE_BLOCK (vivi_code_block_new ());

  for (i = 0; list[i] != NULL; i++) {
    vivi_code_block_add_statement (block, list[i]);
  }

  return VIVI_CODE_STATEMENT (block);
}

// values

static ParseStatus
parse_literal (GScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (check_token (scanner, G_TOKEN_STRING)) {
    *value = vivi_code_constant_new_string (scanner->value.v_string);
    return STATUS_OK;
  } else if (check_token (scanner, G_TOKEN_FLOAT)) {
    *value = vivi_code_constant_new_number (scanner->value.v_float);
    return STATUS_OK;
  } else if (check_token (scanner, TOKEN_TRUE)) {
    *value = vivi_code_constant_new_boolean (TRUE);
    return STATUS_OK;
  } else if (check_token (scanner, TOKEN_FALSE)) {
    *value = vivi_code_constant_new_boolean (FALSE);
    return STATUS_OK;
  } else if (check_token (scanner, TOKEN_NULL)) {
    *value = vivi_code_constant_new_null ();
    return STATUS_OK;
  } else {
    return STATUS_CANCEL;
  }

  *value = vivi_code_get_new_name (scanner->value.v_identifier);

  return STATUS_OK;
}

static ParseStatus
parse_identifier (GScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (scanner, G_TOKEN_IDENTIFIER))
    return STATUS_CANCEL;

  *value = vivi_code_get_new_name (scanner->value.v_identifier);

  return STATUS_OK;
}

// misc

static ParseStatus
parse_variable_declaration (GScanner *scanner, ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *identifier;
  ViviCodeValue *value;

  *statement = NULL;

  status = parse_identifier (scanner, &identifier);
  if (status != STATUS_OK)
    return status;

  if (check_token (scanner, '=')) {
    // TODO
    return STATUS_FAIL;
  } else {
    value = vivi_code_constant_new_undefined ();
  }

  *statement = vivi_code_assignment_new (NULL, identifier, value);
  vivi_code_assignment_set_local (VIVI_CODE_ASSIGNMENT (*statement), TRUE);

  return STATUS_OK;
}

// expression

static ParseStatus
parse_primary_expression (GScanner *scanner, ViviCodeValue **value)
{
  int i;
  ParseStatus status;
  ParseValueFunction functions[] = {
    parse_identifier,
    parse_literal,
    //parse_array_literal,
    //parse_object_literal,
    NULL
  };

  *value = NULL;

  if (check_token (scanner, TOKEN_THIS)) {
    *value = vivi_code_get_new_name ("this");
    return STATUS_OK;
  }

  /*if (check_token (scanner, '(')) {
    return STATUS_OK;
  }*/

  for (i = 0; functions[i] != NULL; i++) {
    status = functions[i] (scanner, value);
    if (status != STATUS_CANCEL)
      return status;
  }

  return STATUS_CANCEL;
}

static ParseStatus
parse_member_expression (GScanner *scanner, ViviCodeValue **value)
{
  ParseStatus status;
  ViviCodeValue *member;

  // TODO: new MemberExpression Arguments

  status = parse_primary_expression (scanner, value);
  //if (status == STATUS_CANCEL)
  //  status = parse_function_expression (scanner, value);

  if (status != 0)
    return status;

  do {
    ViviCodeValue *tmp;

    /*if (check_token (scanner, '[')) {
      if (parse_expression (scanner, &member) != STATUS_OK)
	return STATUS_FAIL;
      if (!check_token (scanner, ']'))
	return STATUS_FAIL;
    } else*/
    if (check_token (scanner, '.')) {
      if (parse_identifier (scanner, &member) != STATUS_OK)
	return STATUS_FAIL;
    } else {
      return STATUS_OK;
    }

    tmp = *value;
    *value = vivi_code_get_new (tmp, VIVI_CODE_VALUE (member));
    g_object_unref (tmp);
    g_object_unref (member);
  } while (TRUE);

  g_assert_not_reached ();
  return STATUS_FAIL;
}

static ParseStatus
parse_new_expression (GScanner *scanner, ViviCodeValue **value)
{
  *value = NULL;

  if (check_token (scanner, TOKEN_NEW)) {
    if (parse_new_expression (scanner, value) != STATUS_OK)
      return STATUS_FAIL;
    if (!VIVI_IS_CODE_FUNCTION_CALL (*value)) {
      ViviCodeValue *tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_code_function_call_new (NULL, tmp);
      g_object_unref (tmp);
    }
    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (*value),
	TRUE);
    return STATUS_OK;
  } else {
    return parse_member_expression (scanner, value);
  }
}

static ParseStatus
parse_left_hand_side_expression (GScanner *scanner, ViviCodeValue **value)
{
  ParseStatus status;

  *value = NULL;

  status = parse_new_expression (scanner, value);
  //if (status == STATUS_CANCEL)
  //  status = parse_call_expression (scanner, value);

  return status;
}

static ParseStatus
parse_postfix_expression (GScanner *scanner, ViviCodeValue **value)
{
  ParseStatus status;

  status = parse_left_hand_side_expression (scanner, value);
  if (status != STATUS_OK)
    return status;

  // FIXME: Don't allow new line here

  /*if (check_token (scanner, TOKEN_PLUSPLUS)) {
    ViviCodeValue *tmp = *value;
    *value = vivi_code_postfix_new (tmp, "++");
    g_object_unref (tmp);
  } else {*/
    return STATUS_OK;
  //}
}

static ParseStatus
parse_unary_expression (GScanner *scanner, ViviCodeValue **value)
{
  ViviCodeValue *tmp;

  *value = NULL;

  if (check_token (scanner, '!')) {
    parse_unary_expression (scanner, value);
    tmp = VIVI_CODE_VALUE (*value);
    *value = vivi_code_unary_new (tmp, '!');
    g_object_unref (tmp);
    return STATUS_OK;
  } else {
    return parse_postfix_expression (scanner, value);
  }
}

static ParseStatus
parse_operator_expression (GScanner *scanner, ViviCodeValue **value)
{
  ParseStatus status;
  ViviCodeValue *left;
  ViviCodeValue *right;

  *value = NULL;

  status = parse_unary_expression (scanner, value);
  if (status != STATUS_OK)
    return status;

  do {
    if (!check_token (scanner, '+'))
      return STATUS_OK;

    if (parse_unary_expression (scanner, &right) != STATUS_OK) {
      g_object_unref (*value);
      *value = NULL;
      return STATUS_FAIL;
    }

    left = VIVI_CODE_VALUE (*value);
    *value = vivi_code_binary_new_name (left, VIVI_CODE_VALUE (right), "+");
    g_object_unref (left);
    g_object_unref (right);
  } while (TRUE);

  g_object_unref (*value);
  *value = NULL;

  return STATUS_FAIL;
}

static ParseStatus
parse_assignment_expression (GScanner *scanner, ViviCodeStatement **statement);

static ParseStatus
parse_conditional_expression (GScanner *scanner, ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *value;
  ViviCodeStatement *if_statement, *else_statement;

  *statement = NULL;
  if_statement = NULL;

  status = parse_operator_expression (scanner, &value);
  if (status != STATUS_OK)
    return status;

  if (!check_token (scanner, '?')) {
    *statement = vivi_code_value_statement_new (VIVI_CODE_VALUE (value));
    g_object_unref (value);
    return STATUS_OK;
  }

  if (parse_assignment_expression (scanner, &if_statement) != STATUS_OK)
    goto fail;

  if (!check_token (scanner, ':'))
    goto fail;

  if (parse_assignment_expression (scanner, &else_statement) != STATUS_OK)
    goto fail;

  *statement = vivi_code_if_new (value);
  vivi_code_if_set_if (VIVI_CODE_IF (*statement), if_statement);
  vivi_code_if_set_else (VIVI_CODE_IF (*statement), else_statement);

  g_object_unref (value);
  g_object_unref (if_statement);
  g_object_unref (else_statement);

  return STATUS_OK;

fail:

  g_object_unref (value);
  if (if_statement != NULL)
    g_object_unref (if_statement);

  return STATUS_FAIL;
}

static ParseStatus
parse_assignment_expression (GScanner *scanner, ViviCodeStatement **statement)
{
  *statement = NULL;

  // TODO

  return parse_conditional_expression (scanner, statement);
}

static ParseStatus
parse_expression (GScanner *scanner, ViviCodeStatement **statement)
{
  ViviCodeStatement **list;
  ParseStatus status;

  *statement = NULL;

  status =
    parse_statement_list (scanner, parse_assignment_expression, &list, ',');
  if (status != STATUS_OK)
    return status;

  *statement = create_block (list);
  free_statement_list (list);

  return STATUS_OK;
}

// statement

static ParseStatus
parse_expression_statement (GScanner *scanner, ViviCodeStatement **statement)
{
  ParseStatus status;

  *statement = NULL;

  g_scanner_peek_next_token (scanner);
  if (scanner->next_token == '{' || scanner->next_token == TOKEN_FUNCTION)
    return STATUS_CANCEL;

  status = parse_expression (scanner, statement);
  if (status != STATUS_OK)
    return status;

  if (!check_token (scanner, ';')) {
    g_object_unref (*statement);
    *statement = NULL;
    return STATUS_FAIL;
  }

  return STATUS_OK;
}

static ParseStatus
parse_empty_statement (GScanner *scanner, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (scanner, ';'))
    return STATUS_CANCEL;

  *statement = vivi_compiler_empty_statement_new ();

  return STATUS_OK;
}

static ParseStatus
parse_statement (GScanner *scanner, ViviCodeStatement **statement);

static ParseStatus
parse_block (GScanner *scanner, ViviCodeStatement **statement)
{
  ViviCodeStatement **list;

  *statement = NULL;

  if (!check_token (scanner, '{'))
    return STATUS_CANCEL;

  g_scanner_peek_next_token (scanner);
  if (scanner->next_token != '}') {
    if (parse_statement_list (scanner, parse_statement, &list, G_TOKEN_NONE)
	!= STATUS_OK)
      return STATUS_FAIL;
  } else {
    list = g_new0 (ViviCodeStatement *, 1);
  }

  if (!check_token (scanner, '}')) {
    free_statement_list (list);
    return STATUS_FAIL;
  }

  *statement = create_block (list);
  free_statement_list (list);

  return STATUS_OK;
}

static ParseStatus
parse_variable_statement (GScanner *scanner, ViviCodeStatement **statement)
{
  ViviCodeStatement **list;
  ParseStatus status;

  *statement = NULL;

  if (!check_token (scanner, TOKEN_VAR)) {
    return STATUS_CANCEL;
  }

  status =
    parse_statement_list (scanner, parse_variable_declaration, &list, ',');
  if (status != STATUS_OK)
    return status;

  if (!check_token (scanner, ';')) {
    free_statement_list (list);
    return STATUS_FAIL;
  }

  *statement = create_block (list);
  free_statement_list (list);

  return STATUS_OK;
}

static ParseStatus
parse_statement (GScanner *scanner, ViviCodeStatement **statement)
{
  int i, status;
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
    status = functions[i] (scanner, statement);
    if (status != STATUS_CANCEL)
      return status;
  }

  return STATUS_CANCEL;
}

// function

static ParseStatus
parse_source_element (GScanner *scanner, ViviCodeStatement **statement);

static ParseStatus
parse_function_declaration (GScanner *scanner, ViviCodeStatement **statement)
{
  //ViviCodeStatement *function;
  ViviCodeValue *identifier;
  ViviCodeValue **arguments;
  ViviCodeStatement **body;

  *statement = NULL;

  identifier = NULL;
  arguments = NULL;
  body = NULL;

  if (!check_token (scanner, TOKEN_FUNCTION))
    return STATUS_CANCEL;

  if (parse_identifier (scanner, &identifier) != STATUS_OK)
    return STATUS_FAIL;

  if (!check_token (scanner, '('))
    goto fail;

  if (parse_value_list (scanner, parse_identifier, &arguments, ',') ==
      STATUS_FAIL)
    goto fail;

  if (!check_token (scanner, ')'))
    goto fail;

  if (!check_token (scanner, '{'))
    goto fail;

  if (parse_statement_list (scanner, parse_source_element, &body, G_TOKEN_NONE)
      == STATUS_FAIL)
    goto fail;

  if (!check_token (scanner, '}'))
    goto fail;

  /*function = vivi_code_function_new (arguments, body);
  *statement = vivi_code_assignment_new (NULL, VIVI_CODE_VALUE (identifier),
      VIVI_CODE_VALUE (function));*/
  *statement = vivi_compiler_empty_statement_new ();

  g_object_unref (identifier);
  if (arguments != NULL)
    free_value_list (arguments);
  if (body != NULL)
    free_statement_list (body);

  return STATUS_OK;

fail:
  if (identifier != NULL)
    g_object_unref (identifier);
  if (arguments != NULL)
    free_value_list (arguments);
  if (body != NULL)
    free_statement_list (body);

  return STATUS_FAIL;
}

// top

static ParseStatus
parse_source_element (GScanner *scanner, ViviCodeStatement **statement)
{
  ParseStatus status;

  *statement = NULL;

  status = parse_function_declaration (scanner, statement);
  if (status == STATUS_CANCEL)
    status = parse_statement (scanner, statement);

  return status;
}

static ParseStatus
parse_program (GScanner *scanner, ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeStatement **list;

  *statement = NULL;

  status = parse_statement_list (scanner, parse_source_element, &list,
      G_TOKEN_NONE);
  if (status != STATUS_OK)
    return status;

  *statement = create_block (list);

  return STATUS_OK;
}

// parsing

static ParseStatus
parse_statement_list (GScanner *scanner, ParseStatementFunction function,
    ViviCodeStatement ***list, guint separator)
{
  GPtrArray *array;
  ViviCodeStatement *statement;
  ParseStatus status;

  g_return_val_if_fail (scanner != NULL, STATUS_FAIL);
  g_return_val_if_fail (function != NULL, STATUS_FAIL);
  g_return_val_if_fail (list != NULL, STATUS_FAIL);

  status = function (scanner, &statement);
  if (status != STATUS_OK)
    return status;

  array = g_ptr_array_new ();

  do {
    g_ptr_array_add (array, statement);

    if (separator != G_TOKEN_NONE && !check_token (scanner, separator))
      break;

    status = function (scanner, &statement);
    if (status == STATUS_FAIL)
      return STATUS_FAIL;
  } while (status == STATUS_OK);
  g_ptr_array_add (array, NULL);

  *list = (ViviCodeStatement **)g_ptr_array_free (array, FALSE);

  return STATUS_OK;
}

static ParseStatus
parse_value_list (GScanner *scanner, ParseValueFunction function,
    ViviCodeValue ***list, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;
  ParseStatus status;

  g_return_val_if_fail (scanner != NULL, STATUS_FAIL);
  g_return_val_if_fail (function != NULL, STATUS_FAIL);
  g_return_val_if_fail (list != NULL, STATUS_FAIL);

  status = function (scanner, &value);
  if (status != STATUS_OK)
    return status;

  array = g_ptr_array_new ();

  do {
    g_ptr_array_add (array, value);

    if (separator != G_TOKEN_NONE && !check_token (scanner, separator))
      break;

    status = function (scanner, &value);
    if (status == STATUS_FAIL)
      return STATUS_FAIL;
  } while (status == STATUS_OK);
  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);

  return STATUS_OK;
}

// public

ViviCodeStatement *
vivi_compile_text (const char *text, gsize len, const char *input_name)
{
  GScanner *scanner;
  ViviCodeStatement *statement;
  ParseStatus status;
  int i;

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

  status = parse_program (scanner, &statement);
  g_assert ((status == STATUS_OK && VIVI_IS_CODE_STATEMENT (statement)) ||
	(status != STATUS_OK && statement == NULL));

  g_scanner_destroy (scanner);

  return statement;
}
