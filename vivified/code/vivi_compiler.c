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
  ERROR_TOKEN_IDENTIFIER,
  ERROR_TOKEN_PROPERTY_NAME,
  ERROR_TOKEN_PRIMARY_EXPRESSION,
  ERROR_TOKEN_ITERATION_STATEMENT,
  ERROR_TOKEN_EXPRESSION_STATEMENT,
  ERROR_TOKEN_STATEMENT
};

static const struct {
  guint				token;
  const char *			name;
} error_names[] = {
  { ERROR_TOKEN_LITERAL, "LITERAL" },
  { ERROR_TOKEN_IDENTIFIER, "IDENTIFIER" },
  { ERROR_TOKEN_PROPERTY_NAME, "PROPERTY NAME" },
  { ERROR_TOKEN_PRIMARY_EXPRESSION, "PRIMARY EXPRESSION" },
  { ERROR_TOKEN_ITERATION_STATEMENT, "ITERATION STATEMENT" },
  { ERROR_TOKEN_EXPRESSION_STATEMENT, "EXPRESSION STATEMENT" },
  { ERROR_TOKEN_STATEMENT, "STATEMENT" },
  { TOKEN_LAST, NULL }
};

typedef enum {
  STATUS_CANCEL = -1,
  STATUS_OK = 0,
  STATUS_FAIL = 1
} ParseStatus;

typedef struct {
  GSList			*labels;
} ParseLevel;

typedef struct {
  ViviCompilerScanner *		scanner;
  gboolean			unexpected_line_terminator;
  guint				expected[2];
  const char *			custom_error;

  GSList *			levels; // ParseLevel, earlier levels
  ParseLevel *			level;  // current level
} ParseData;

#define FAIL_OR(x, y) (data->expected[0] = (x), data->expected[1] = (y), STATUS_FAIL)
#define FAIL_LINE_TERMINATOR_OR(x, y) (data->unexpected_line_terminator = TRUE, FAIL_OR(x,y))
#define FAIL(x) FAIL_OR(x,TOKEN_NONE)
#define FAIL_CHILD(x) STATUS_FAIL
#define FAIL_CUSTOM(x) (data->custom_error = (x), STATUS_FAIL)
#define CANCEL_OR(x, y) (data->expected[0] = (x), data->expected[1] = (y), STATUS_CANCEL)
#define CANCEL(x) CANCEL_OR(x, TOKEN_NONE)
#define CANCEL_CUSTOM(x) (data->custom_error = (x), STATUS_CANCEL)

typedef ParseStatus (*ParseValueFunction) (ParseData *data, ViviCodeValue **value);
typedef ParseStatus (*ParseValueStatementFunction) (ParseData *data, ViviCodeValue **value, ViviCodeStatement **statement);
typedef ParseStatus (*ParseStatementFunction) (ParseData *data, ViviCodeStatement **statement);

static ParseStatus
parse_statement_list (ParseData *data, ParseStatementFunction function, ViviCodeStatement **statement, guint separator);

static ParseStatus
parse_value_statement_list (ParseData *data,
    ParseValueStatementFunction function, ViviCodeValue ***list,
    ViviCodeStatement **statement, guint separator);

static ParseStatus
parse_value_list (ParseData *data, ParseValueFunction function,
    ViviCodeValue ***list, guint separator);

// helpers

static gboolean
check_line_terminator (ParseData *data)
{
  vivi_compiler_scanner_peek_next_token (data->scanner);
  return data->scanner->next_line_terminator;
}

static gboolean
check_token (ParseData *data, ViviCompilerScannerToken token)
{
  vivi_compiler_scanner_peek_next_token (data->scanner);
  if (data->scanner->next_token != token)
    return FALSE;
  vivi_compiler_scanner_get_next_token (data->scanner);
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

G_GNUC_WARN_UNUSED_RESULT static ViviCodeStatement *
vivi_compiler_combine_statements (guint count, ...)
{
  va_list args;
  ViviCodeBlock *block;
  guint i;

  if (count == 0)
    return NULL;

  va_start (args, count);
  block = VIVI_CODE_BLOCK (vivi_code_block_new ());
  for (i = 0; i < count; i++) {
    ViviCodeStatement *statement = va_arg (args, ViviCodeStatement *);

    if (statement == NULL)
      continue;

    g_assert (VIVI_IS_CODE_STATEMENT (statement));

    vivi_code_block_add_statement (block, statement);
    g_object_unref (statement);
  }
  va_end (args);

  if (vivi_code_block_get_n_statements (block) == 0) {
    g_object_unref (block);
    return NULL;
  }

  if (vivi_code_block_get_n_statements (block) == 1) {
    ViviCodeStatement *statement =
      g_object_ref (vivi_code_block_get_statement (block, 0));
    g_object_unref (block);
    return statement;
  }

  return VIVI_CODE_STATEMENT (block);
}

static ViviCodeValue *
vivi_compiler_function_call_new (ViviCodeValue *name)
{
  ViviCodeValue *value;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (name), NULL);

  value = NULL;

  if (VIVI_IS_CODE_GET (name)) {
    ViviCodeGet *get = VIVI_CODE_GET (name);

    if (get->from != NULL) {
      value = g_object_ref (get->from);
      name = g_object_ref (get->name);
    }
  }

  if (VIVI_IS_CODE_GET (name)) {
    ViviCodeGet *get = VIVI_CODE_GET (name);

    if (get->from == NULL)
      name = g_object_ref (get->name);
  }

  return vivi_code_function_call_new (value, name);
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

static gboolean
vivi_compiler_value_is_left_hand_side (ViviCodeValue *value)
{
  // FIXME: Correct?
  return VIVI_IS_CODE_GET (value);
}

static gboolean
vivi_compiler_value_is_identifier (ViviCodeValue *value)
{
  if (!VIVI_IS_CODE_GET (value))
    return FALSE;
  return VIVI_IS_CODE_CONSTANT (VIVI_CODE_GET (value)->name);
}

static void
vivi_compiler_start_level (ParseData *data)
{
  g_return_if_fail (data != NULL);

  if (data->level != NULL)
    data->levels = g_slist_prepend (data->levels, data->level);
  data->level = g_new0 (ParseLevel, 1);
}

static void
vivi_compiler_end_level (ParseData *data)
{
  GSList *iter;

  g_return_if_fail (data != NULL);
  g_return_if_fail (data->level != NULL);

  for (iter = data->level->labels; iter != NULL; iter = iter->next) {
    g_object_unref (VIVI_CODE_LABEL (iter->data));
  }
  g_slist_free (data->level->labels);

  g_free (data->level);

  if (data->levels != NULL) {
    data->level = data->levels->data;
    data->levels = g_slist_delete_link (data->levels, data->levels);
  } else {
    data->level = NULL;
  }
}

static gboolean
vivi_compiler_add_label (ParseData *data, ViviCodeLabel *label)
{
  GSList *iter;

  g_return_val_if_fail (data->level != NULL, FALSE);

  for (iter = data->level->labels; iter != NULL; iter = iter->next) {
    if (g_str_equal (vivi_code_label_get_name (VIVI_CODE_LABEL (iter->data)),
	  vivi_code_label_get_name (label)))
      return FALSE;
  }

  data->level->labels =
    g_slist_prepend (data->level->labels, g_object_ref (label));

  return TRUE;
}

// values

static ParseStatus
parse_null_literal (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_NULL))
    return CANCEL (TOKEN_NULL);

  *value = vivi_code_constant_new_null ();
  return STATUS_OK;
}

static ParseStatus
parse_boolean_literal (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_BOOLEAN))
    return CANCEL (TOKEN_BOOLEAN);

  *value = vivi_code_constant_new_boolean (data->scanner->value.v_boolean);
  return STATUS_OK;
}

static ParseStatus
parse_numeric_literal (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_NUMBER))
    return CANCEL (TOKEN_NUMBER);

  *value = vivi_code_constant_new_number (data->scanner->value.v_number);
  return STATUS_OK;
}

static ParseStatus
parse_string_literal (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_STRING))
    return CANCEL (TOKEN_STRING);

  *value = vivi_code_constant_new_string (data->scanner->value.v_string);
  return STATUS_OK;
}

static ParseStatus
parse_literal (ParseData *data, ViviCodeValue **value)
{
  ParseStatus status;
  int i;
  ParseValueFunction functions[] = {
    parse_null_literal,
    parse_boolean_literal,
    parse_numeric_literal,
    parse_string_literal,
    NULL
  };

  *value = NULL;

  for (i = 0; functions[i] != NULL; i++) {
    status = functions[i] (data, value);
    if (status != STATUS_CANCEL)
      return status;
  }

  return CANCEL (ERROR_TOKEN_LITERAL);
}

static ParseStatus
parse_identifier (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_IDENTIFIER))
    return CANCEL (TOKEN_IDENTIFIER);

  *value = vivi_code_get_new_name (data->scanner->value.v_identifier);

  return STATUS_OK;
}

static ParseStatus
parse_property_name (ParseData *data, ViviCodeValue **value)
{
  ParseStatus status;
  int i;
  ParseValueFunction functions[] = {
    parse_identifier,
    parse_string_literal,
    parse_numeric_literal,
    NULL
  };

  *value = NULL;

  for (i = 0; functions[i] != NULL; i++) {
    status = functions[i] (data, value);
    if (status != STATUS_CANCEL)
      return status;
  }

  return CANCEL (ERROR_TOKEN_PROPERTY_NAME);
}

static ParseStatus
parse_assignment_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement);

static ParseStatus
parse_object_literal (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;

  *value = NULL;
  *statement = NULL;

  if (!check_token (data, TOKEN_BRACE_LEFT))
    return CANCEL (TOKEN_BRACE_LEFT);

  *value = vivi_code_init_object_new ();

  if (!check_token (data, TOKEN_BRACE_RIGHT)) {
    do {
      ViviCodeValue *property, *initializer;
      ViviCodeStatement *statement_new;

      status = parse_property_name (data, &property);
      if (status != STATUS_OK) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL_CHILD (status);
      }

      if (!check_token (data, TOKEN_COLON)) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL (TOKEN_COLON);
      }

      status = parse_assignment_expression (data, &initializer,
	  &statement_new);
      if (status != STATUS_OK) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL_CHILD (status);
      }

      *statement =
	vivi_compiler_combine_statements (2, *statement, statement_new);

      vivi_code_init_object_add_variable (VIVI_CODE_INIT_OBJECT (*value),
	  property, initializer);
    } while (check_token (data, TOKEN_COMMA));
  }

  if (!check_token (data, TOKEN_BRACE_RIGHT)) {
    g_object_unref (*value);
    *value = NULL;
    if (*statement != NULL) {
      g_object_unref (*statement);
      *statement = NULL;
    }
    return FAIL (TOKEN_BRACE_RIGHT);
  }

  return STATUS_OK;
}

// misc

static ParseStatus
parse_variable_declaration (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *identifier, *value;
  ViviCodeStatement *assignment, *statement_right;

  *statement = NULL;

  status = parse_identifier (data, &identifier);
  if (status != STATUS_OK)
    return status;

  if (check_token (data, TOKEN_ASSIGN)) {
    status = parse_assignment_expression (data, &value, &statement_right);
    if (status != STATUS_OK) {
      g_object_unref (identifier);
      return FAIL_CHILD (status);
    }
  } else {
    value = vivi_code_constant_new_undefined ();
    statement_right = NULL;
  }

  assignment = vivi_compiler_assignment_new (identifier, value);
  vivi_code_assignment_set_local (VIVI_CODE_ASSIGNMENT (assignment), TRUE);

  *statement =
    vivi_compiler_combine_statements (2, statement_right, assignment);

  return STATUS_OK;
}

// expression

static ParseStatus
parse_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement);

static ParseStatus
parse_primary_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;
  int i;
  ParseValueFunction functions[] = {
    parse_identifier,
    parse_literal,
    //parse_array_literal,
    NULL
  };

  *value = NULL;
  *statement = NULL;

  if (check_token (data, TOKEN_THIS)) {
    *value = vivi_code_get_new_name ("this");
    return STATUS_OK;
  }

  if (check_token (data, TOKEN_PARENTHESIS_LEFT)) {
    status = parse_expression (data, value, statement);
    if (status != STATUS_OK)
      return FAIL_CHILD (status);
    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      g_object_unref (*value);
      *value = NULL;
      return FAIL (TOKEN_PARENTHESIS_RIGHT);
    }
    return STATUS_OK;
  }


  status = parse_object_literal (data, value, statement);
  if (status != STATUS_CANCEL)
    return status;

  for (i = 0; functions[i] != NULL; i++) {
    status = functions[i] (data, value);
    if (status != STATUS_CANCEL)
      return status;
  }

  return CANCEL (ERROR_TOKEN_PRIMARY_EXPRESSION);
}

static ParseStatus
parse_function_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement);

static ParseStatus
parse_member_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *member;
  ViviCodeStatement *statement_member;

  *value = NULL;
  *statement = NULL;

  // TODO: new MemberExpression Arguments

  status = parse_primary_expression (data, value, statement);
  if (status == STATUS_CANCEL)
    status = parse_function_expression (data, value, statement);

  if (status != STATUS_OK)
    return status;

  do {
    ViviCodeValue *tmp;

    if (check_token (data, TOKEN_BRACKET_LEFT)) {
      status = parse_expression (data, &member, &statement_member);
      if (status != STATUS_OK) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL_CHILD (status);
      }

      *statement = vivi_compiler_combine_statements (2, *statement,
	  statement_member);

      if (!check_token (data, TOKEN_BRACKET_RIGHT)) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL (TOKEN_BRACKET_RIGHT);
      }
    } else if (check_token (data, TOKEN_DOT)) {
      status = parse_identifier (data, &member);
      if (status != STATUS_OK)
	return FAIL_CHILD (status);
    } else {
      return STATUS_OK;
    }

    tmp = *value;
    *value = vivi_compiler_get_new (tmp, member);
    g_object_unref (tmp);
    g_object_unref (member);
  } while (TRUE);

  g_assert_not_reached ();
}

static ParseStatus
parse_new_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;

  *value = NULL;
  *statement = NULL;

  if (check_token (data, TOKEN_NEW)) {
    status = parse_new_expression (data, value, statement);
    if (status != STATUS_OK)
      return FAIL_CHILD (status);
    if (!VIVI_IS_CODE_FUNCTION_CALL (*value)) {
      ViviCodeValue *tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_code_function_call_new (NULL, tmp);
      g_object_unref (tmp);
    }
    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (*value),
	TRUE);
    return STATUS_OK;
  } else {
    return parse_member_expression (data, value, statement);
  }
}

static ParseStatus
parse_left_hand_side_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;
  int i;
  ViviCodeValue *name;
  ViviCodeValue **arguments;
  ViviCodeStatement *argument_statement;

  *value = NULL;
  *statement = NULL;

  if (check_token (data, TOKEN_NEW)) {
    status = parse_new_expression (data, value, statement);
    if (status != STATUS_OK)
      return FAIL_CHILD (status);
    if (!VIVI_IS_CODE_FUNCTION_CALL (*value)) {
      ViviCodeValue *tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_code_function_call_new (NULL, tmp);
      g_object_unref (tmp);
    }
    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (*value),
	TRUE);
    return STATUS_OK;
  }

  status = parse_member_expression (data, value, statement);
  if (status != STATUS_OK)
    return status;

  while (TRUE) {
    // TODO: member expressions?
    if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
      break;

    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      status = parse_value_statement_list (data, parse_assignment_expression,
	  &arguments, &argument_statement, TOKEN_COMMA);
      if (status != STATUS_OK) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL_CHILD (status);
      }

      *statement =
	vivi_compiler_combine_statements (2, *statement, argument_statement);

      if (!check_token (data, TOKEN_PARENTHESIS_RIGHT)) {
	g_object_unref (*value);
	*value = NULL;
	if (*statement != NULL) {
	  g_object_unref (*statement);
	  *statement = NULL;
	}
	return FAIL (TOKEN_PARENTHESIS_RIGHT);
      }
    } else {
      arguments = NULL;
    }

    name = *value;
    *value = vivi_compiler_function_call_new (name);
    g_object_unref (name);

    if (arguments != NULL) {
      for (i = 0; arguments[i] != NULL; i++) {
	vivi_code_function_call_add_argument (VIVI_CODE_FUNCTION_CALL (*value),
	    arguments[i]);
      }
      free_value_list (arguments);
    }
  }

  return STATUS_OK;
}

static ParseStatus
parse_postfix_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *operation, *one, *temporary;
  const char *operator;

  *value = NULL;
  *statement = NULL;

  status = parse_left_hand_side_expression (data, value, statement);
  if (status != STATUS_OK)
    return status;

  if (check_line_terminator (data)) {
    g_object_unref (*value);
    *value = NULL;
    if (*statement != NULL) {
      g_object_unref (*statement);
      *statement = NULL;
    }
    return FAIL_LINE_TERMINATOR_OR (TOKEN_INCREASE, TOKEN_DESCREASE);
  }

  if (check_token (data, TOKEN_INCREASE)) {
    operator = "+";
  } else if (check_token (data, TOKEN_DESCREASE)) {
    operator = "-";
  } else {
    return STATUS_OK;
  }

  if (!vivi_compiler_value_is_left_hand_side (*value)) {
    g_object_unref (*value);
    *value = NULL;
    if (*statement != NULL) {
      g_object_unref (*statement);
      *statement = NULL;
    }
    return CANCEL_CUSTOM ("INCREASE/DECREASE not allowed here");
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

  return STATUS_OK;
}

static ParseStatus
parse_unary_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *tmp, *one;
  const char *operator;

  *value = NULL;

  operator = NULL;

  vivi_compiler_scanner_peek_next_token (data->scanner);
  switch ((guint)data->scanner->next_token) {
    /*case TOKEN_DELETE:
    case TOKEN_VOID:
    case TOKEN_TYPEOF:*/
    case TOKEN_INCREASE:
      operator = "+";
      // fall through
    case TOKEN_DESCREASE:
      if (!operator) operator = "-";

      vivi_compiler_scanner_get_next_token (data->scanner);
      status = parse_unary_expression (data, value, statement);
      if (status != STATUS_OK)
	return FAIL_CHILD (status);

      if (!vivi_compiler_value_is_left_hand_side (*value))
	return CANCEL_CUSTOM ("INCREASE/DECREASE not allowed here");

      one = vivi_code_constant_new_number (1);
      tmp = vivi_code_binary_new_name (*value, one, operator);
      g_object_unref (one);

      *statement = vivi_compiler_combine_statements (2, *statement,
	  vivi_compiler_assignment_new (*value, tmp));
      g_object_unref (tmp);

      return STATUS_OK;
    /*case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BITWISE_NOT:*/
    case TOKEN_LOGICAL_NOT:
      vivi_compiler_scanner_get_next_token (data->scanner);
      status = parse_unary_expression (data, value, statement);
      if (status != STATUS_OK)
	return FAIL_CHILD (status);
      tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_code_unary_new (tmp, '!');
      g_object_unref (tmp);
      return STATUS_OK;
    default:
      return parse_postfix_expression (data, value, statement);
  }
}

typedef enum {
  PASS_ALWAYS,
  PASS_LOGICAL_OR,
  PASS_LOGICAL_AND
} ParseOperatorPass;

static ParseStatus
parse_operator_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement, const ViviCompilerScannerToken *tokens,
    ParseOperatorPass pass, ParseValueStatementFunction next_parse_function)
{
  ParseStatus status;
  int i;
  ViviCodeValue *left, *right;
  ViviCodeStatement *statement_right;

  *value = NULL;
  *statement = NULL;

  status = next_parse_function (data, value, statement);
  if (status != STATUS_OK)
    return status;

  for (i = 0; tokens[i] != STATUS_OK; i++) {
    while (check_token (data, tokens[i])) {
      status = next_parse_function (data, &right, &statement_right);
      if (status != STATUS_OK) {
	g_object_unref (*value);
	*value = NULL;
	return FAIL_CHILD (status);
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

  return STATUS_OK;
}

static ParseStatus
parse_multiplicative_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_MULTIPLY,
    TOKEN_DIVIDE, TOKEN_REMAINDER, TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_unary_expression);
}

static ParseStatus
parse_additive_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_multiplicative_expression);
}

static ParseStatus
parse_shift_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT, TOKEN_SHIFT_RIGHT_UNSIGNED, TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_additive_expression);
}

static ParseStatus
parse_relational_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN, /*TOKEN_LESS_THAN_OR_EQUAL,
    TOKEN_EQUAL_OR_GREATER_THAN, TOKEN_INSTANCEOF, TOKEN_IN,*/ TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_shift_expression);
}

static ParseStatus
parse_equality_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_EQUAL,
    /*TOKEN_NOT_EQUAL,*/ TOKEN_STRICT_EQUAL, /*TOKEN_NOT_STRICT_EQUAL,*/
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_relational_expression);
}

static ParseStatus
parse_bitwise_and_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_BITWISE_AND,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_equality_expression);
}

static ParseStatus
parse_bitwise_xor_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_BITWISE_XOR,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_bitwise_and_expression);
}

static ParseStatus
parse_bitwise_or_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_BITWISE_OR,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_bitwise_xor_expression);
}

static ParseStatus
parse_logical_and_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_LOGICAL_AND,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_LOGICAL_AND, parse_bitwise_or_expression);
}

static ParseStatus
parse_logical_or_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviCompilerScannerToken tokens[] = { TOKEN_LOGICAL_OR,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_LOGICAL_OR, parse_logical_and_expression);
}

static ParseStatus
parse_conditional_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;
  //ViviCodeStatement *if_statement, *else_statement;

  *value = NULL;

  status = parse_logical_or_expression (data, value, statement);
  if (status != STATUS_OK)
    return status;

#if 0
  if (!check_token (data, TOKEN_QUESTION_MARK)) {
    *statement = vivi_code_value_statement_new (VIVI_CODE_VALUE (value));
    g_object_unref (value);
    return STATUS_OK;
  }

  status = parse_assignment_expression (data, &if_statement);
  if (status != STATUS_OK) {
    g_object_unref (value);
    return FAIL_CHILD (status);
  }

  if (!check_token (data, TOKEN_COLON)) {
    g_object_unref (value);
    g_object_unref (if_statement);
    return TOKEN_COLON;
  }

  status = parse_assignment_expression (data, &else_statement);
  if (status != STATUS_OK) {
    g_object_unref (value);
    g_object_unref (if_statement);
    return FAIL_CHILD (status);
  }

  *statement = vivi_code_if_new (value);
  vivi_code_if_set_if (VIVI_CODE_IF (*statement), if_statement);
  vivi_code_if_set_else (VIVI_CODE_IF (*statement), else_statement);

  g_object_unref (value);
  g_object_unref (if_statement);
  g_object_unref (else_statement);
#endif

  return STATUS_OK;
}

static ParseStatus
parse_assignment_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *right;
  ViviCodeStatement *assignment, *statement_right;
  const char *operator;

  *value = NULL;
  *statement = NULL;

  status = parse_conditional_expression (data, value, statement);
  if (status != STATUS_OK)
    return status;

  if (!vivi_compiler_value_is_left_hand_side (*value))
    return STATUS_OK;

  operator = NULL;

  vivi_compiler_scanner_peek_next_token (data->scanner);
  switch ((int)data->scanner->next_token) {
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
      vivi_compiler_scanner_get_next_token (data->scanner);
      status = parse_assignment_expression (data, &right,
	  &statement_right);
      if (status != STATUS_OK) {
	g_object_unref (*value);
	*value = NULL;
	return FAIL_CHILD (status);
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
      return STATUS_OK;
  }


  return STATUS_OK;
}

static ParseStatus
parse_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeStatement *statement_one;
  ParseStatus status;

  *statement = NULL;

  status = parse_assignment_expression (data, value, &statement_one);
  if (status != STATUS_OK)
    return status;

  while (TRUE) {
    *statement =
      vivi_compiler_combine_statements (2, *statement, statement_one);

    if (!check_token (data, TOKEN_COMMA))
      break;

    status = parse_assignment_expression (data, value, &statement_one);
    if (status != STATUS_OK) {
      g_object_unref (*value);
      *value = NULL;
      g_object_unref (*statement);
      *statement = NULL;
      return FAIL_CHILD (status);
    }
  }

  return STATUS_OK;
}

// statement

static ParseStatus
parse_statement (ParseData *data, ViviCodeStatement **statement);

static ParseStatus
parse_continue_statement (ParseData *data, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (data, TOKEN_CONTINUE))
    return CANCEL (TOKEN_CONTINUE);

  if (check_line_terminator (data))
    return FAIL_LINE_TERMINATOR_OR (TOKEN_SEMICOLON, ERROR_TOKEN_IDENTIFIER);

  *statement = vivi_code_continue_new ();

  if (!check_token (data, TOKEN_SEMICOLON)) {
    g_object_unref (*statement);
    *statement = NULL;
    return FAIL_CUSTOM ("Handling of label in continue has not been implemented yet");

    /* if (!check_token (data, TOKEN_SEMICOLON)) {
      g_object_unref (*statement);
      *statement = NULL;
      return FAIL (TOKEN_SEMICOLON);
    } */
  }

  return STATUS_OK;
}

static ParseStatus
parse_break_statement (ParseData *data, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (data, TOKEN_BREAK))
    return CANCEL (TOKEN_BREAK);

  if (check_line_terminator (data))
    return FAIL_LINE_TERMINATOR_OR (TOKEN_SEMICOLON, ERROR_TOKEN_IDENTIFIER);

  if (!check_token (data, TOKEN_SEMICOLON)) {
    return FAIL_CUSTOM ("Handling of label in break has not been implemented yet");

    /* if (!check_token (data, TOKEN_SEMICOLON)) {
      g_object_unref (*statement);
      *statement = NULL;
      return FAIL (TOKEN_SEMICOLON);
    } */
  } else {
    *statement = vivi_code_break_new ();
  }

  return STATUS_OK;
}

static ParseStatus
parse_return_statement (ParseData *data, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (data, TOKEN_RETURN))
    return CANCEL (TOKEN_RETURN);

  // FIXME: no LineTerminator here

  *statement = vivi_code_return_new ();

  if (!check_token (data, TOKEN_SEMICOLON)) {
    ParseStatus status;
    ViviCodeValue *value;
    ViviCodeStatement *expression_statement;

    status = parse_expression (data, &value, &expression_statement);
    if (status != STATUS_OK) {
      g_object_unref (*statement);
      *statement = NULL;
      return FAIL_CHILD (status);
    }

    vivi_code_return_set_value (VIVI_CODE_RETURN (*statement), value);
    g_object_unref (value);

    *statement =
      vivi_compiler_combine_statements (2, expression_statement, *statement);

    if (!check_token (data, TOKEN_SEMICOLON)) {
      g_object_unref (*statement);
      *statement = NULL;
      return FAIL (TOKEN_SEMICOLON);
    }
  }

  return STATUS_OK;
}

static ParseStatus
parse_iteration_statement (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *condition;
  ViviCodeStatement *pre_statement, *condition_statement, *loop_statement;

  *statement = NULL;

  // TODO: for, do while

  pre_statement = NULL;
  condition_statement = NULL;

  if (check_token (data, TOKEN_DO)) {
    status = parse_statement (data, &loop_statement);
    if (status != STATUS_OK)
      return FAIL_CHILD (status);

    if (!check_token (data, TOKEN_WHILE)) {
      g_object_unref (loop_statement);
      return FAIL (TOKEN_WHILE);
    }

    if (!check_token (data, TOKEN_PARENTHESIS_LEFT)) {
      g_object_unref (loop_statement);
      return FAIL (TOKEN_PARENTHESIS_LEFT);
    }

    status = parse_expression (data, &condition, &condition_statement);
    if (status != STATUS_OK) {
      g_object_unref (loop_statement);
      return FAIL_CHILD (status);
    }

    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      g_object_unref (loop_statement);
      g_object_unref (condition);
      if (condition_statement != NULL)
	g_object_unref (condition_statement);
      return FAIL (TOKEN_PARENTHESIS_LEFT);
    }

    if (!check_token (data, TOKEN_SEMICOLON)) {
      g_object_unref (loop_statement);
      g_object_unref (condition);
      if (condition_statement != NULL)
	g_object_unref (condition_statement);
      return FAIL (TOKEN_PARENTHESIS_LEFT);
    }

    pre_statement = g_object_ref (loop_statement);
  } else if (check_token (data, TOKEN_WHILE)) {
    if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
      return FAIL (TOKEN_PARENTHESIS_LEFT);

    status = parse_expression (data, &condition, &condition_statement);
    if (status != STATUS_OK)
      return FAIL_CHILD (status);

    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      g_object_unref (condition);
      if (condition_statement != NULL)
	g_object_unref (condition_statement);
      return FAIL (TOKEN_PARENTHESIS_RIGHT);
    }

    status = parse_statement (data, &loop_statement);
    if (status != STATUS_OK) {
      g_object_unref (condition);
      if (condition_statement != NULL)
	g_object_unref (condition_statement);
      return FAIL_CHILD (status);
    }
  } else if (check_token (data, TOKEN_FOR)) {
    ViviCodeValue *pre_value;
    ViviCodeStatement *post_statement;

    if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
      return FAIL (TOKEN_PARENTHESIS_LEFT);

    if (check_token (data, TOKEN_VAR)) {
      // FIXME: no in
      status = parse_statement_list (data, parse_variable_declaration,
	  &pre_statement, TOKEN_COMMA);
      if (status != STATUS_OK)
	return FAIL_CHILD (status);
      // FIXME: ugly
      // If there was only one VariableDeclaration, get the name for pre_value
      g_assert (VIVI_IS_CODE_BLOCK (pre_statement));
      if (vivi_code_block_get_n_statements (VIVI_CODE_BLOCK (pre_statement))
	  == 1) {
	ViviCodeAssignment *assignment = VIVI_CODE_ASSIGNMENT (
	    vivi_code_block_get_statement (VIVI_CODE_BLOCK (pre_statement), 0));
	g_assert (assignment->from == NULL);
	pre_value = vivi_code_get_new (NULL, assignment->name);
      } else {
	pre_value = NULL;
      }
    } else {
      if (!check_token (data, TOKEN_SEMICOLON)) {
	// FIXME: no in
	status = parse_expression (data, &pre_value, &pre_statement);
	if (status != STATUS_OK)
	  return FAIL_CHILD (status);
      } else {
	pre_value = NULL;
	pre_statement = NULL;
      }
    }

    if (check_token (data, TOKEN_SEMICOLON)) {
      if (pre_value != NULL)
	g_object_unref (pre_value);
      if (!check_token (data, TOKEN_SEMICOLON)) {
	status = parse_expression (data, &condition, &condition_statement);
	if (status != STATUS_OK)
	  return FAIL_CHILD (status);

	if (!check_token (data, TOKEN_SEMICOLON)) {
	  if (pre_statement != NULL)
	    g_object_unref (pre_statement);
	  g_object_unref (condition);
	  g_object_unref (condition_statement);
	  return FAIL (TOKEN_SEMICOLON);
	}
      }

      status = parse_expression (data, &pre_value, &post_statement);
      if (status != STATUS_OK) {
	if (pre_statement != NULL)
	  g_object_unref (pre_statement);
	g_object_unref (condition);
	g_object_unref (condition_statement);
	return FAIL_CHILD (status);
      }
      g_object_unref (pre_value);
    // FIXME: or one in declaration
    } else if (pre_value != NULL && check_token (data, TOKEN_IN)) {
      post_statement = NULL;

      if (!vivi_compiler_value_is_left_hand_side (pre_value)) {
	g_object_unref (pre_value);
	if (pre_statement != NULL)
	  g_object_unref (pre_statement);
	return FAIL_CUSTOM ("Invalid left hand side expression for in");
      }

      g_object_unref (pre_value);
      if (pre_statement != NULL)
	g_object_unref (pre_statement);
      return FAIL_CUSTOM ("for (... in ...) has not been implemented yet");
    } else {
      ViviCompilerScannerToken fail_token;

      if (pre_value != NULL) {
	fail_token = TOKEN_IN;
	g_object_unref (pre_value);
      } else {
	fail_token = TOKEN_NONE;
      }

      if (pre_statement != NULL)
	g_object_unref (pre_statement);

      return FAIL_OR (TOKEN_SEMICOLON, fail_token);
    }

    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      if (pre_statement != NULL)
	g_object_unref (pre_statement);
      g_object_unref (condition);
      g_object_unref (condition_statement);
      g_object_unref (post_statement);
      return FAIL (TOKEN_PARENTHESIS_RIGHT);
    }

    status = parse_statement (data, &loop_statement);
    if (status != STATUS_OK) {
      if (pre_statement != NULL)
	g_object_unref (pre_statement);
      g_object_unref (condition);
      g_object_unref (condition_statement);
      g_object_unref (post_statement);
      return FAIL_CHILD (status);
    }

    loop_statement = vivi_compiler_combine_statements (2,
	loop_statement, post_statement);
  } else {
    return CANCEL (ERROR_TOKEN_ITERATION_STATEMENT);
  }

  if (condition_statement != NULL) {
    pre_statement = vivi_compiler_combine_statements (2,
	pre_statement, g_object_ref (condition_statement));
    loop_statement = vivi_compiler_combine_statements (2,
	loop_statement, g_object_ref (condition_statement));
    g_object_unref (condition_statement);
  }

  *statement = vivi_code_loop_new ();
  vivi_code_loop_set_condition (VIVI_CODE_LOOP (*statement), condition);
  g_object_unref (condition);
  vivi_code_loop_set_statement (VIVI_CODE_LOOP (*statement), loop_statement);
  g_object_unref (loop_statement);

  *statement = vivi_compiler_combine_statements (2, pre_statement, *statement);

  return STATUS_OK;
}

static ParseStatus
parse_if_statement (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *condition;
  ViviCodeStatement *pre_statement, *if_statement, *else_statement;

  *statement = NULL;

  if (!check_token (data, TOKEN_IF))
    return CANCEL (TOKEN_IF);

  if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
    return FAIL (TOKEN_PARENTHESIS_LEFT);

  status = parse_expression (data, &condition, &pre_statement);
  if (status != STATUS_OK)
    return FAIL_CHILD (status);

  if (!check_token (data, TOKEN_PARENTHESIS_RIGHT)) {
    g_object_unref (condition);
    if (pre_statement != NULL) {
      g_object_unref (pre_statement);
      pre_statement = NULL;
    }
    return FAIL (TOKEN_PARENTHESIS_RIGHT);
  }

  status = parse_statement (data, &if_statement);
  if (status != STATUS_OK) {
    g_object_unref (condition);
    if (pre_statement != NULL) {
      g_object_unref (pre_statement);
      pre_statement = NULL;
    }
    return FAIL_CHILD (status);
  }

  if (check_token (data, TOKEN_ELSE)) {
    status = parse_statement (data, &else_statement);
    if (status != STATUS_OK) {
      g_object_unref (condition);
      if (pre_statement != NULL) {
	g_object_unref (pre_statement);
	pre_statement = NULL;
      }
      g_object_unref (if_statement);
      return FAIL_CHILD (status);
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

  return STATUS_OK;
}

static ParseStatus
parse_expression_statement (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *value;

  *statement = NULL;

  vivi_compiler_scanner_peek_next_token (data->scanner);
  if (data->scanner->next_token == TOKEN_BRACE_LEFT ||
      data->scanner->next_token == TOKEN_FUNCTION)
    return CANCEL (ERROR_TOKEN_EXPRESSION_STATEMENT);

  status = parse_expression (data, &value, statement);
  if (status != STATUS_OK)
    return status;

  // check for label
  if (*statement == NULL && vivi_compiler_value_is_identifier (value)) {
    if (check_token (data, TOKEN_COLON)) {
      *statement = vivi_code_label_new (vivi_code_constant_get_variable_name (
	    VIVI_CODE_CONSTANT (VIVI_CODE_GET (value)->name)));
      if (!vivi_compiler_add_label (data, VIVI_CODE_LABEL (*statement)))
	return FAIL_CUSTOM ("Same label name used twice");
      return STATUS_OK;
    }
  }

  if (!check_token (data, TOKEN_SEMICOLON)) {
    g_object_unref (value);
    if (*statement != NULL) {
      g_object_unref (*statement);
      *statement = NULL;
    }
    return FAIL (TOKEN_SEMICOLON);
  }

  *statement = vivi_compiler_combine_statements (2, *statement,
      vivi_code_value_statement_new (value));
  g_object_unref (value);

  return STATUS_OK;
}

static ParseStatus
parse_empty_statement (ParseData *data, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (data, TOKEN_SEMICOLON))
    return CANCEL (TOKEN_SEMICOLON);

  *statement = vivi_compiler_empty_statement_new ();

  return STATUS_OK;
}

static ParseStatus
parse_block (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;

  *statement = NULL;

  if (!check_token (data, TOKEN_BRACE_LEFT))
    return CANCEL (TOKEN_BRACE_LEFT);

  vivi_compiler_scanner_peek_next_token (data->scanner);
  if (data->scanner->next_token != TOKEN_BRACE_RIGHT) {
    status =
      parse_statement_list (data, parse_statement, statement, STATUS_OK);
    if (status != STATUS_OK)
      return FAIL_CHILD (status);
  } else {
    *statement = vivi_code_block_new ();
  }

  if (!check_token (data, TOKEN_BRACE_RIGHT)) {
    g_object_unref (*statement);
    *statement = NULL;
    return FAIL (TOKEN_BRACE_RIGHT);
  }

  return STATUS_OK;
}

static ParseStatus
parse_variable_statement (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;

  *statement = NULL;

  if (!check_token (data, TOKEN_VAR))
    return CANCEL (TOKEN_VAR);

  status = parse_statement_list (data, parse_variable_declaration, statement,
      TOKEN_COMMA);
  if (status != STATUS_OK)
    return FAIL_CHILD (status);

  if (!check_token (data, TOKEN_SEMICOLON)) {
    g_object_unref (*statement);
    *statement = NULL;
    return FAIL (TOKEN_SEMICOLON);
  }

  return STATUS_OK;
}

static ParseStatus
parse_statement (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;
  int i;
  ParseStatementFunction functions[] = {
    parse_block,
    parse_variable_statement,
    parse_empty_statement,
    parse_expression_statement,
    parse_if_statement,
    parse_iteration_statement,
    parse_continue_statement,
    parse_break_statement,
    parse_return_statement,
    //parse_with_statement,
    //parse_switch_statement,
    //parse_throw_statement,
    //parse_try_statement,
    NULL
  };

  *statement = NULL;

  for (i = 0; functions[i] != NULL; i++) {
    status = functions[i] (data, statement);
    if (status != STATUS_CANCEL)
      return status;
  }

  return CANCEL (ERROR_TOKEN_STATEMENT);
}

// function

static ParseStatus
parse_source_element (ParseData *data, ViviCodeStatement **statement);

static ParseStatus
parse_function_definition (ParseData *data, ViviCodeValue **function,
    ViviCodeValue **identifier, gboolean identifier_required)
{
  ParseStatus status;
  ViviCodeValue **arguments;
  ViviCodeStatement *body;

  *function = NULL;
  *identifier = NULL;

  arguments = NULL;
  body = NULL;

  if (!check_token (data, TOKEN_FUNCTION))
    return CANCEL (TOKEN_FUNCTION);

  status = parse_identifier (data, identifier);
  if (status == STATUS_FAIL ||
      (identifier_required && status == STATUS_CANCEL))
    return FAIL_CHILD (status);

  if (!check_token (data, TOKEN_PARENTHESIS_LEFT)) {
    g_object_unref (*identifier);
    return FAIL (TOKEN_PARENTHESIS_LEFT);
  }

  status = parse_value_list (data, parse_identifier, &arguments, TOKEN_COMMA);
  if (status == STATUS_FAIL) {
    g_object_unref (*identifier);
    return STATUS_FAIL;
  }

  if (!check_token (data, TOKEN_PARENTHESIS_RIGHT)) {
    g_object_unref (*identifier);
    if (arguments != NULL)
      free_value_list (arguments);
    return FAIL (TOKEN_PARENTHESIS_RIGHT);
  }

  if (!check_token (data, TOKEN_BRACE_LEFT)) {
    g_object_unref (*identifier);
    if (arguments != NULL)
      free_value_list (arguments);
    return FAIL (TOKEN_BRACE_LEFT);
  }

  vivi_compiler_start_level (data);

  status = parse_statement_list (data, parse_source_element, &body, STATUS_OK);

  vivi_compiler_end_level (data);

  if (status == STATUS_FAIL) {
    g_object_unref (*identifier);
    if (arguments != NULL)
      free_value_list (arguments);
    return STATUS_FAIL;
  }

  if (!check_token (data, TOKEN_BRACE_RIGHT)) {
    g_object_unref (*identifier);
    if (arguments != NULL)
      free_value_list (arguments);
    g_object_unref (body);
    return FAIL (TOKEN_BRACE_RIGHT);
  }

  *function = vivi_code_function_new ();
  vivi_code_function_set_body (VIVI_CODE_FUNCTION (*function), body);
  if (arguments != NULL)
    free_value_list (arguments);
  g_object_unref (body);

  return STATUS_OK;
}

static ParseStatus
parse_function_declaration (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *function, *identifier;

  *statement = NULL;

  status = parse_function_definition (data, &function, &identifier, TRUE);
  if (status != STATUS_OK)
    return status;

  *statement = vivi_compiler_assignment_new (identifier, function);
  g_object_unref (identifier);
  g_object_unref (function);

  return STATUS_OK;
}

static ParseStatus
parse_function_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ParseStatus status;
  ViviCodeValue *identifier;

  *statement = NULL;

  status = parse_function_definition (data, value, &identifier, FALSE);
  if (status != STATUS_OK)
    return status;

  if (identifier != NULL) {
    *statement = vivi_compiler_assignment_new (identifier, *value);
    g_object_unref (identifier);
  }

  return STATUS_OK;
}

// top

static ParseStatus
parse_source_element (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;

  *statement = NULL;

  status = parse_function_declaration (data, statement);
  if (status == STATUS_CANCEL)
    status = parse_statement (data, statement);

  return status;
}

static ParseStatus
parse_program (ParseData *data, ViviCodeStatement **statement)
{
  ParseStatus status;

  g_assert (data->level == NULL);
  vivi_compiler_start_level (data);

  *statement = NULL;

  status =
    parse_statement_list (data, parse_source_element, statement, STATUS_OK);
  if (status != STATUS_OK)
    return FAIL_CHILD (status);

  if (!check_token (data, TOKEN_EOF)) {
    g_object_unref (*statement);
    *statement = NULL;
    status = parse_statement (data, statement);
    return FAIL_CHILD (status);
  }


  vivi_compiler_end_level (data);
  g_assert (data->level == NULL);

  return STATUS_OK;
}

// parsing

static ParseStatus
parse_statement_list (ParseData *data, ParseStatementFunction function,
    ViviCodeStatement **block, guint separator)
{
  ViviCodeStatement *statement;
  ParseStatus status;

  g_assert (data != NULL);
  g_assert (function != NULL);
  g_assert (block != NULL);

  *block = NULL;

  status = function (data, &statement);
  if (status != STATUS_OK)
    return status;

  *block = vivi_code_block_new ();

  do {
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (*block), statement);
    g_object_unref (statement);

    if (separator != STATUS_OK && !check_token (data, separator))
      break;

    status = function (data, &statement);
    if (status == STATUS_FAIL) {
      g_object_unref (*block);
      *block = NULL;
      return STATUS_FAIL;
    }
  } while (status == STATUS_OK);

  return STATUS_OK;
}

static ParseStatus
parse_value_statement_list (ParseData *data,
    ParseValueStatementFunction function, ViviCodeValue ***list,
    ViviCodeStatement **statement, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;
  ViviCodeStatement *statement_one;
  ParseStatus status;

  g_assert (data != NULL);
  g_assert (function != NULL);
  g_assert (list != NULL);
  g_assert (statement != NULL);

  *list = NULL;
  *statement = NULL;

  status = function (data, &value, statement);
  if (status != STATUS_OK)
    return status;

  array = g_ptr_array_new ();

  do {
    g_ptr_array_add (array, value);

    if (separator != TOKEN_NONE && !check_token (data, separator))
      break;

    status = function (data, &value, &statement_one);
    if (status != STATUS_OK) {
      if (status == STATUS_FAIL || separator != TOKEN_NONE)
	return FAIL_CHILD (status);
    } else {
      *statement =
	vivi_compiler_combine_statements (2, *statement, statement_one);
    }
  } while (status == STATUS_OK);
  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);

  return STATUS_OK;
}

static ParseStatus
parse_value_list (ParseData *data, ParseValueFunction function,
    ViviCodeValue ***list, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;
  ParseStatus status;

  g_assert (data != NULL);
  g_assert (function != NULL);
  g_assert (list != NULL);

  *list = NULL;

  status = function (data, &value);
  if (status != STATUS_OK)
    return status;

  array = g_ptr_array_new ();

  do {
    g_ptr_array_add (array, value);

    if (separator != STATUS_OK && !check_token (data, separator))
      break;

    status = function (data, &value);
    if (status == STATUS_FAIL)
      return STATUS_FAIL;
  } while (status == STATUS_OK);
  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);

  return STATUS_OK;
}

// public

ViviCodeStatement *
vivi_compile_file (FILE *file, const char *input_name)
{
  ParseData data;
  ViviCodeStatement *statement;
  ParseStatus status;

  g_return_val_if_fail (file != NULL, NULL);

  data.scanner = vivi_compiler_scanner_new (file);
  data.unexpected_line_terminator = FALSE;
  data.expected[0] = TOKEN_NONE;
  data.expected[1] = TOKEN_NONE;
  data.custom_error = NULL;
  data.levels = NULL;
  data.level = NULL;

  status = parse_program (&data, &statement);
  g_assert ((status == STATUS_OK && VIVI_IS_CODE_STATEMENT (statement)) ||
	(status != STATUS_OK && statement == NULL));
  g_assert (status >= 0);

  if (status != STATUS_OK) {
    // FIXME: multiple expected tokens
    vivi_compiler_scanner_get_next_token (data.scanner);
    if (data.custom_error != NULL) {
      g_printerr ("%s\n", data.custom_error);
    } else if (data.expected[0] < TOKEN_LAST) {
      vivi_compiler_scanner_unexp_token (data.scanner, data.expected[0]);
    } else {
      guint i;
      const char *name;

      name = NULL;
      for (i = 0; error_names[i].token != TOKEN_LAST; i++) {
	if (error_names[i].token == data.expected[0]) {
	  name = error_names[i].name;
	  break;
	}
      }

      g_assert (name != NULL);

      vivi_compiler_scanner_unexp_token_custom (data.scanner, name);
    }
  }

  g_object_unref (data.scanner);

  return statement;
}
