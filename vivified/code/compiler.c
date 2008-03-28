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
#include <swfdec/swfdec.h>

#include "vivi_code_assignment.h"
#include "vivi_code_binary.h"
#include "vivi_code_block.h"
#include "vivi_code_break.h"
#include "vivi_code_constant.h"
#include "vivi_code_continue.h"
#include "vivi_code_empty.h"
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

typedef enum {
  STATUS_FAIL = -1,
  STATUS_OK = 0,
  STATUS_CANCEL = 1
} ParseStatus;

enum {
  TOKEN_FUNCTION = G_TOKEN_LAST + 1,
  TOKEN_PLUSPLUS = G_TOKEN_LAST + 2,
  TOKEN_MINUSMINUS = G_TOKEN_LAST + 3,
  TOKEN_NEW = G_TOKEN_LAST + 4
};

typedef enum {
  SYMBOL_NONE,
  // top
  SYMBOL_SOURCE_ELEMENT,
  // function
  SYMBOL_FUNCTION_DECLARATION,
  // statement
  SYMBOL_STATEMENT,
  SYMBOL_BLOCK,
  SYMBOL_VARIABLE_STATEMENT,
  SYMBOL_EMPTY_STATEMENT,
  SYMBOL_EXPRESSION_STATEMENT,
  SYMBOL_IF_STATEMENT,
  SYMBOL_ITERATION_STATEMENT,
  SYMBOL_CONTINUE_STATEMENT,
  SYMBOL_BREAK_STATEMENT,
  SYMBOL_RETURN_STATEMENT,
  SYMBOL_WITH_STATEMENT,
  SYMBOL_LABELLED_STATEMENT,
  SYMBOL_SWITCH_STATEMENT,
  SYMBOL_THROW_STATEMENT,
  SYMBOL_TRY_STATEMENT,
  // expression
  SYMBOL_ASSIGNMENT_EXPRESSION,
  SYMBOL_CONDITIONAL_EXPRESSION,
  SYMBOL_LEFT_HAND_SIDE_EXPRESSION,
  SYMBOL_OPERATOR_EXPRESSION,
  SYMBOL_UNARY_EXPRESSION,
  SYMBOL_POSTFIX_EXPRESSION,
  SYMBOL_NEW_EXPRESSION,
  SYMBOL_CALL_EXPRESSION,
  SYMBOL_MEMBER_EXPRESSION,
  SYMBOL_FUNCTION_EXPRESSION,
  SYMBOL_PRIMARY_EXPRESSION,
  // misc
  SYMBOL_IDENTIFIER,
  SYMBOL_ASSIGNMENT_OPERATOR,
  SYMBOL_ARGUMENTS,
} ParseSymbol;

typedef ParseStatus (*ParseFunction) (GScanner *scanner, ViviCodeToken **token);

static ParseStatus
parse (GScanner *scanner, ParseSymbol symbol, ViviCodeToken **token);

static ParseStatus
parse_list (GScanner *scanner, ParseSymbol symbol, ViviCodeToken ***list,
    guint separator);

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
free_list (ViviCodeToken **list)
{
  int i;

  for (i = 0; list[i] != NULL; i++) {
    g_object_unref (list[i]);
  }
  g_free (list);
}

static ViviCodeBlock *
create_block (ViviCodeToken **list)
{
  ViviCodeBlock *block;
  int i;

  block = VIVI_CODE_BLOCK (vivi_code_block_new ());

  for (i = 0; list[i] != NULL; i++) {
    vivi_code_block_add_statement (block, VIVI_CODE_STATEMENT (list[i]));
  }

  return block;
}

// top

static ParseStatus
parse_source_element (GScanner *scanner, ViviCodeToken **token)
{
  ParseStatus status;

  *token = NULL;

  status = parse (scanner, SYMBOL_FUNCTION_DECLARATION, token);

  if (status == STATUS_CANCEL)
    status = parse (scanner, SYMBOL_STATEMENT, token);

  return status;
}

// function

static ParseStatus
parse_function_declaration (GScanner *scanner, ViviCodeToken **token)
{
  ViviCodeToken *function, *identifier;
  ViviCodeToken **arguments, **body;

  *token = NULL;

  identifier = NULL;
  arguments = NULL;
  body = NULL;

  if (!check_token (scanner, TOKEN_FUNCTION))
    return STATUS_CANCEL;

  if (parse (scanner, SYMBOL_IDENTIFIER, &identifier) != STATUS_OK)
    return STATUS_FAIL;

  if (!check_token (scanner, '('))
    goto fail;

  if (parse_list (scanner, SYMBOL_IDENTIFIER, &arguments, ',') == STATUS_FAIL)
    goto fail;

  if (!check_token (scanner, ')'))
    goto fail;

  if (!check_token (scanner, '{'))
    goto fail;

  if (parse_list (scanner, SYMBOL_SOURCE_ELEMENT, &body, G_TOKEN_NONE)
      == STATUS_FAIL)
    goto fail;

  if (!check_token (scanner, '}'))
    goto fail;

  //function = vivi_code_function_new (arguments, body);
  *token = VIVI_CODE_TOKEN (
      vivi_code_assignment_new (NULL, VIVI_CODE_VALUE (identifier),
      VIVI_CODE_VALUE (function)));

  g_object_unref (identifier);
  if (arguments != NULL)
    free_list (arguments);
  free_list (body);

  return STATUS_OK;

fail:
  if (identifier != NULL)
    g_object_unref (identifier);
  if (arguments != NULL)
    free_list (arguments);
  if (body != NULL)
    free_list (body);

  return STATUS_FAIL;
}

// statement

static ParseStatus
parse_statement (GScanner *scanner, ViviCodeToken **token)
{
  int i, status;
  ParseSymbol options[] = {
    SYMBOL_BLOCK,
    SYMBOL_VARIABLE_STATEMENT,
    SYMBOL_EMPTY_STATEMENT,
    SYMBOL_EXPRESSION_STATEMENT,
    SYMBOL_IF_STATEMENT,
    SYMBOL_ITERATION_STATEMENT,
    SYMBOL_CONTINUE_STATEMENT,
    SYMBOL_BREAK_STATEMENT,
    SYMBOL_RETURN_STATEMENT,
    SYMBOL_WITH_STATEMENT,
    SYMBOL_LABELLED_STATEMENT,
    SYMBOL_SWITCH_STATEMENT,
    SYMBOL_THROW_STATEMENT,
    SYMBOL_TRY_STATEMENT,
    SYMBOL_NONE
  };

  *token = NULL;

  for (i = 0; options[i] != SYMBOL_NONE; i++) {
    status = parse (scanner, options[i], token);
    if (status != STATUS_CANCEL)
      return status;
  }

  return STATUS_CANCEL;
}

static ParseStatus
parse_block (GScanner *scanner, ViviCodeToken **token)
{
  ViviCodeToken **list;

  *token = NULL;

  if (!check_token (scanner, '{'))
    return STATUS_CANCEL;

  list = NULL;

  g_scanner_peek_next_token (scanner);
  if (scanner->next_token != '}') {
    if (parse_list (scanner, SYMBOL_STATEMENT, &list, G_TOKEN_NONE)
	!= STATUS_OK)
      return STATUS_FAIL;
  }

  if (!check_token (scanner, '}')) {
    if (list != NULL)
      free_list (list);
    return STATUS_FAIL;
  }

  *token = VIVI_CODE_TOKEN (create_block (list));
  free_list (list);

  return STATUS_OK;
}

static ParseStatus
parse_empty_statement (GScanner *scanner, ViviCodeToken **token)
{
  *token = NULL;

  if (!check_token (scanner, ';'))
    return STATUS_CANCEL;

  *token = VIVI_CODE_TOKEN (vivi_code_empty_new ());

  return STATUS_OK;
}

static ParseStatus
parse_expression_statement (GScanner *scanner, ViviCodeToken **token)
{
  ViviCodeToken **list;
  ParseStatus status;

  *token = NULL;

  g_scanner_peek_next_token (scanner);
  if (scanner->next_token == '{' || scanner->next_token == TOKEN_FUNCTION)
    return STATUS_CANCEL;

  status = parse_list (scanner, SYMBOL_ASSIGNMENT_EXPRESSION, &list, ',');

  if (status == STATUS_OK) {
    if (list[1] == NULL) {
      *token = list[0];
    } else {
      *token = VIVI_CODE_TOKEN (create_block (list));
    }
  }

  return status;
}

// expression

static ParseStatus
parse_assignment_expression (GScanner *scanner, ViviCodeToken **token)
{
  *token = NULL;

  // TODO

  return parse (scanner, SYMBOL_CONDITIONAL_EXPRESSION, token);
}

static ParseStatus
parse_conditional_expression (GScanner *scanner, ViviCodeToken **token)
{
  ParseStatus status;
  ViviCodeToken *value;
  ViviCodeToken *first, *second;

  *token = NULL;
  first = NULL;

  status = parse (scanner, SYMBOL_OPERATOR_EXPRESSION, &value);
  if (status != STATUS_OK)
    return status;

  if (!check_token (scanner, '?')) {
    *token = VIVI_CODE_TOKEN (
	vivi_code_value_statement_new (VIVI_CODE_VALUE (value)));
    g_object_unref (value);
    return STATUS_OK;
  }

  if (parse (scanner, SYMBOL_ASSIGNMENT_EXPRESSION, &first) != STATUS_OK)
    goto fail;

  if (!check_token (scanner, ':'))
    goto fail;

  if (parse (scanner, SYMBOL_ASSIGNMENT_EXPRESSION, &second) != STATUS_OK)
    goto fail;

  //*token = vivi_code_condional_new (VIVI_CODE_VALUE (value),
  //    VIVI_CODE_STATEMENT (first), VIVI_CODE_STATEMENT (second));

  g_object_unref (value);
  g_object_unref (first);
  g_object_unref (second);

  return STATUS_OK;

fail:

  g_object_unref (value);
  if (first != NULL)
    g_object_unref (first);

  return STATUS_FAIL;
}

static ParseStatus
parse_operator_expression (GScanner *scanner, ViviCodeToken **token)
{
  ParseStatus status;
  ViviCodeValue *left;
  ViviCodeToken *right;

  *token = NULL;

  status = parse (scanner, SYMBOL_UNARY_EXPRESSION, token);
  if (status != STATUS_OK)
    return status;

  do {
    if (!check_token (scanner, '+'))
      return STATUS_OK;

    if (parse (scanner, SYMBOL_UNARY_EXPRESSION, &right) != STATUS_OK) {
      g_object_unref (*token);
      *token = NULL;
      return STATUS_FAIL;
    }

    left = VIVI_CODE_VALUE (token);
    *token = VIVI_CODE_TOKEN (vivi_code_binary_new_name (left,
	  VIVI_CODE_VALUE (right), "+"));
    g_object_unref (left);
    g_object_unref (right);
  } while (TRUE);

  g_object_unref (*token);
  *token = NULL;

  return STATUS_FAIL;
}

static ParseStatus
parse_unary_expression (GScanner *scanner, ViviCodeToken **token)
{
  ViviCodeValue *value;

  *token = NULL;

  if (check_token (scanner, '!')) {
    parse (scanner, SYMBOL_UNARY_EXPRESSION, token);
    value = VIVI_CODE_VALUE (*token);
    *token = VIVI_CODE_TOKEN (vivi_code_unary_new (value, '!'));
    g_object_unref (value);
    return STATUS_OK;
  } else {
    return parse (scanner, SYMBOL_POSTFIX_EXPRESSION, token);
  }
}

static ParseStatus
parse_postfix_expression (GScanner *scanner, ViviCodeToken **token)
{
  //ViviCodeValue *value;
  ParseStatus status;

  status = parse (scanner, SYMBOL_LEFT_HAND_SIDE_EXPRESSION, token);
  if (status != STATUS_OK)
    return status;

  // FIXME: Don't allow new line here

  /*if (check_token (scanner, TOKEN_PLUSPLUS)) {
    value = VIVI_CODE_VALUE (*token);
    *token = VIVI_CODE_TOKEN (vivi_code_postfix_new (value, "++"));
    g_object_unref (value);
  } else {*/
    return STATUS_OK;
  //}
}

static ParseStatus
parse_left_hand_side_expression (GScanner *scanner, ViviCodeToken **token)
{
  ParseStatus status;

  *token = NULL;

  status = parse (scanner, SYMBOL_NEW_EXPRESSION, token);
  if (status == STATUS_CANCEL)
    status = parse (scanner, SYMBOL_CALL_EXPRESSION, token);

  return status;
}

static ParseStatus
parse_new_expression (GScanner *scanner, ViviCodeToken **token)
{
  *token = NULL;

  if (check_token (scanner, TOKEN_NEW)) {
    if (parse (scanner, SYMBOL_NEW_EXPRESSION, token) != STATUS_OK)
      return STATUS_FAIL;
    if (!VIVI_IS_CODE_FUNCTION_CALL (*token)) {
      ViviCodeValue *value = VIVI_CODE_VALUE (*token);
      *token = VIVI_CODE_TOKEN (vivi_code_function_call_new (NULL, value));
      g_object_unref (value);
    }
    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (*token),
	TRUE);
    return STATUS_OK;
  } else {
    return parse (scanner, SYMBOL_MEMBER_EXPRESSION, token);
  }
}

static ParseStatus
parse_member_expression (GScanner *scanner, ViviCodeToken **token)
{
  ParseStatus status;
  ViviCodeValue *value;
  ViviCodeToken *member;

  // TODO: new MemberExpression Arguments

  status = parse (scanner, SYMBOL_PRIMARY_EXPRESSION, token);
  if (status == STATUS_CANCEL)
    status = parse (scanner, SYMBOL_FUNCTION_EXPRESSION, token);

  if (status != 0)
    return status;

  do {
    if (check_token (scanner, '[')) {
      if (parse (scanner, SYMBOL_EXPRESSION, &member) != STATUS_OK)
	return STATUS_FAIL;
      if (!check_token (scanner, ']'))
	return STATUS_FAIL;
    } else if (check_token (scanner, '.')) {
      if (parse (scanner, SYMBOL_IDENTIFIER, &member) != STATUS_OK)
	return STATUS_FAIL;
    } else {
      return STATUS_OK;
    }

    value = VIVI_CODE_VALUE (token)
    *token = VIVI_CODE_TOKEN (
	vivi_code_get_new (value, VIVI_CODE_VALUE (member)));
    g_object_unref (value);
    g_object_unref (member);
  } while (TRUE);

  g_assert_not_reached ();
  return STATUS_FAIL;
}

static ParseStatus
parse_primary_expression (GScanner *scanner, ViviCodeToken **token)
{
  return parse (scanner, SYMBOL_IDENTIFIER, token);
}

// misc.

static ParseStatus
parse_identifier (GScanner *scanner, ViviCodeToken **token)
{
  *token = NULL;

  if (!check_token (scanner, G_TOKEN_IDENTIFIER))
    return STATUS_CANCEL;

  *token = VIVI_CODE_TOKEN (vivi_code_get_new_name (scanner->value));

  return STATUS_OK;
}

static ParseStatus
parse_assignment_operator (GScanner *scanner)
{
  if (!check_token (scanner, '='))
    return STATUS_CANCEL;

  return STATUS_OK;
}

// parsing

static const struct {
  ParseSymbol		id;
  const char *		name;
  ParseFunction		parse;
} symbols[] = {
  // top
  { SYMBOL_SOURCE_ELEMENT, "SourceElement", parse_source_element },
  // function
  { SYMBOL_FUNCTION_DECLARATION, "FunctionDeclaration",
    parse_function_declaration },
  // statement
  { SYMBOL_STATEMENT, "Statement", parse_statement },
  { SYMBOL_BLOCK, "Block", parse_block },
  { SYMBOL_EMPTY_STATEMENT, "EmptyStatement", parse_empty_statement },
  { SYMBOL_EXPRESSION_STATEMENT, "ExpressionStatement",
    parse_expression_statement },
  // expression
  { SYMBOL_EXPRESSION, "Expression", parse_expression },
  { SYMBOL_ASSIGNMENT_EXPRESSION, "AssigmentExpression",
    parse_assignment_expression },
  { SYMBOL_CONDITIONAL_EXPRESSION, "ConditionalExpression",
    parse_conditional_expression },
  { SYMBOL_OPERATOR_EXPRESSION, "OperatorExpression",
    parse_operator_expression },
  { SYMBOL_UNARY_EXPRESSION, "UnaryExpression", parse_unary_expression },
  { SYMBOL_POSTFIX_EXPRESSION, "PostfixExpression", parse_postfix_expression },
  { SYMBOL_LEFT_HAND_SIDE_EXPRESSION, "LeftHandSideExpression",
    parse_left_hand_side_expression },
  { SYMBOL_NEW_EXPRESSION, "NewExpression", parse_new_expression },
  { SYMBOL_MEMBER_EXPRESSION, "MemberExpression", parse_member_expression },
  { SYMBOL_PRIMARY_EXPRESSION, "PrimaryExpression", parse_primary_expression },
  // misc
  { SYMBOL_IDENTIFIER, "Identifier", parse_identifier },
  { SYMBOL_ASSIGNMENT_OPERATOR, "AssignmentOperator",
    parse_assignment_operator },
  { SYMBOL_NONE, NULL, NULL }
};

static ParseSymbol
parse (GScanner *scanner, ParseSymbol symbol)
{
  int i;

  for (i = 0; symbols[i].id != SYMBOL_NONE; i++) {
    if (symbols[i].id == symbol) {
      ParseSymbol ret = symbols[i].parse (scanner);
      if (ret != STATUS_CANCEL)
	g_print (":%i: %s\n", ret, symbols[i].name);
      return ret;
    }
  }

  //g_assert_not_reached ();
  return STATUS_CANCEL;
}

static ParseSymbol
parse_list (GScanner *scanner, ParseSymbol symbol, ViviCodeToken ***list,
    guint separator)
{
  GPtrArray *array;
  ViviCodeToken *token;
  ParseSymbol ret;

  ret = parse (scanner, symbol, &token);
  if (ret != STATUS_OK)
    return ret;

  array = g_ptr_array_new ();

  do {
    g_ptr_array_add (array, token);

    if (separator != G_TOKEN_NONE && !check_token (scanner, separator))
      break;

    ret = parse (scanner, symbol, &token);
    if (ret == STATUS_FAIL)
      return STATUS_FAIL;
  } while (ret == STATUS_OK);
  g_ptr_array_add (array, NULL);

  *list = g_ptr_array_free (array, FALSE);

  return STATUS_OK;
}

// main

int
main (int argc, char *argv[])
{
  GScanner *scanner;
  char *test_text;
  gsize test_text_len;
  int ret;

  if (argc < 2) {
    g_print ("Usage!\n");
    return 1;
  }

  if (!g_file_get_contents (argv[1], &test_text, &test_text_len, NULL)) {
    g_printerr ("Couldn't open file %s", argv[1]);
    return -1;
  }

  scanner = g_scanner_new (NULL);

  scanner->config->numbers_2_int = TRUE;
  scanner->config->int_2_float = TRUE;
  scanner->config->symbol_2_token = TRUE;

  g_scanner_set_scope (scanner, 0);
  g_scanner_scope_add_symbol (scanner, 0, "function",
      GINT_TO_POINTER(TOKEN_FUNCTION));
  g_scanner_scope_add_symbol (scanner, 0, "++",
      GINT_TO_POINTER(TOKEN_PLUSPLUS));
  g_scanner_scope_add_symbol (scanner, 0, "--",
      GINT_TO_POINTER(TOKEN_MINUSMINUS));
  g_scanner_scope_add_symbol (scanner, 0, "new", GINT_TO_POINTER(TOKEN_NEW));

  scanner->input_name = argv[1];
  g_scanner_input_text (scanner, test_text, test_text_len);


  ret = parse_list (scanner, SYMBOL_SOURCE_ELEMENT);
  g_print ("%i: %i, %i\n", ret,
      g_scanner_cur_line (scanner), g_scanner_cur_position (scanner));

  g_scanner_peek_next_token (scanner);
  while (scanner->next_token != G_TOKEN_EOF &&
      scanner->next_token != G_TOKEN_ERROR)
  {
    g_scanner_get_next_token (scanner);
    g_print (":: %i, %i :: ", g_scanner_cur_line (scanner),
	g_scanner_cur_position (scanner));
    switch ((int)scanner->token) {
      case G_TOKEN_SYMBOL:
	g_print ("SYMBOL\n");
	break;
      case G_TOKEN_FLOAT:
	g_print ("Primitive: %f\n", scanner->value.v_float);
	break;
      case G_TOKEN_STRING:
	g_print ("Primitive: \"%s\"\n", scanner->value.v_string);
	break;
      case G_TOKEN_NONE:
	g_print ("NONE\n");
	break;
      case G_TOKEN_IDENTIFIER:
	g_print ("Identifier: %s\n", scanner->value.v_identifier);
	break;
      default:
	if (scanner->token > 0 && scanner->token < 256) {
	  g_print ("%c\n", scanner->token);
	} else {
	  g_print ("TOKEN %i\n", scanner->token);
	}
	break;
    }
    g_scanner_peek_next_token (scanner);
  }

  g_scanner_destroy (scanner);

  return 0;
}
