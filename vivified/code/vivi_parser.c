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

#include "vivi_parser.h"

#include "vivi_parser_scanner.h"

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
#include "vivi_code_init_array.h"
#include "vivi_code_init_object.h"
#include "vivi_code_loop.h"
#include "vivi_code_return.h"
#include "vivi_code_throw.h"
#include "vivi_code_trace.h"
#include "vivi_code_unary.h"
#include "vivi_code_value_statement.h"
#include "vivi_compiler_empty_statement.h"
#include "vivi_compiler_get_temporary.h"
#include "vivi_compiler_goto_name.h"

#include "vivi_code_text_printer.h"

enum {
  ERROR_TOKEN_LITERAL = TOKEN_LAST + 1,
  ERROR_TOKEN_IDENTIFIER,
  ERROR_TOKEN_PROPERTY_NAME,
  ERROR_TOKEN_PRIMARY_EXPRESSION,
  ERROR_TOKEN_EXPRESSION,
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
  { ERROR_TOKEN_EXPRESSION, "EXPRESSION" },
  { ERROR_TOKEN_ITERATION_STATEMENT, "ITERATION STATEMENT" },
  { ERROR_TOKEN_EXPRESSION_STATEMENT, "EXPRESSION STATEMENT" },
  { ERROR_TOKEN_STATEMENT, "STATEMENT" },
  { TOKEN_LAST, NULL }
};

typedef struct {
  GSList			*labels;
  GSList			*gotos;
} ParseLevel;

typedef struct {
  ViviParserScanner *		scanner;

  guint				error_count;

  char *			cancel_error;

  GSList *			levels; // ParseLevel, earlier levels
  ParseLevel *			level;  // current level
} ParseData;

typedef gboolean (*ParseValueFunction) (ParseData *data, ViviCodeValue **value);
typedef gboolean (*ParseValueStatementFunction) (ParseData *data, ViviCodeValue **value, ViviCodeStatement **statement);
typedef gboolean (*ParseStatementFunction) (ParseData *data, ViviCodeStatement **statement);

static gboolean
parse_statement_list (ParseData *data, ParseStatementFunction function, ViviCodeStatement **statement, guint separator);

static gboolean
parse_value_statement_list (ParseData *data,
    ParseValueStatementFunction function, ViviCodeValue ***list,
    ViviCodeStatement **statement, guint separator);

static gboolean
parse_value_list (ParseData *data, ParseValueFunction function,
    ViviCodeValue ***list, guint separator);

// helpers

static const char *
vivi_parser_token_name (guint token)
{
  if (token < TOKEN_LAST) {
    return vivi_parser_scanner_token_name (token);
  } else {
    guint i;
    const char *name;

    name = NULL;
    for (i = 0; error_names[i].token != TOKEN_LAST; i++) {
      if (error_names[i].token == token) {
	name = error_names[i].name;
	break;
      }
    }

    g_assert (name != NULL);

    return name;
  }
}

static void
vivi_parser_error (ParseData *data, const char *format, ...)
{
  va_list args;
  char *message;

  g_return_if_fail (data != NULL);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  message = g_strdup_vprintf (format, args);
  va_end (args);

  g_printerr (":error: %s\n", message);

  g_free (message);

  data->error_count++;
}

static void
vivi_parser_cancel (ParseData *data, const char *format, ...)
{
  va_list args;
  char *message;

  g_return_if_fail (data != NULL);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  message = g_strdup_vprintf (format, args);
  va_end (args);

  if (data->cancel_error != NULL)
    g_free (data->cancel_error);
  data->cancel_error = message;
}

static char *
vivi_parser_unexpected_token (ParseData *data, guint expected,
    guint unexpected1, guint unexpected2)
{
  return g_strdup_printf ("Expected %s before %s or %s\n",
      vivi_parser_token_name (expected), vivi_parser_token_name (unexpected1),
      vivi_parser_token_name (unexpected2));
}

static void
vivi_parser_fail_child (ParseData *data)
{
  if (data->cancel_error != NULL) {
    vivi_parser_error (data, "%s", data->cancel_error);
    g_free (data->cancel_error);
    data->cancel_error = NULL;
  } else {
    g_assert (data->error_count > 0);
  }
}

static gboolean
check_line_terminator (ParseData *data)
{
  vivi_parser_scanner_peek_next_token (data->scanner);
  return data->scanner->next_line_terminator;
}

static gboolean
check_token (ParseData *data, ViviParserScannerToken token)
{
  vivi_parser_scanner_peek_next_token (data->scanner);
  if (data->scanner->next_token != token)
    return FALSE;
  vivi_parser_scanner_get_next_token (data->scanner);
  return TRUE;
}

static gboolean
check_automatic_semicolon (ParseData *data)
{
  if (check_token (data, TOKEN_SEMICOLON))
    return TRUE;
  if (check_line_terminator (data))
    return TRUE;

  vivi_parser_scanner_peek_next_token (data->scanner);
  if (data->scanner->next_token == TOKEN_BRACE_LEFT ||
      data->scanner->next_token == TOKEN_EOF)
    return TRUE;

  return FALSE;
}

static gboolean
check_restricted_semicolon (ParseData *data)
{
  if (check_token (data, TOKEN_SEMICOLON))
    return TRUE;
  if (check_line_terminator (data))
    return TRUE;

  return FALSE;
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
vivi_parser_join_statements (ViviCodeStatement *one, ViviCodeStatement *two)
{

  if (one == NULL) {
    return two;
  } else if (two == NULL) {
    return one;
  }

  if (VIVI_IS_CODE_BLOCK (one)) {
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (one), two);
    g_object_unref (two);
    return one;
  } else if (VIVI_IS_CODE_BLOCK (two)) {
    vivi_code_block_insert_statement (VIVI_CODE_BLOCK (two), 0, one);
    g_object_unref (one);
    return two;
  } else {
    ViviCodeStatement *block = vivi_code_block_new ();
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), one);
    g_object_unref (one);
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), two);
    g_object_unref (two);
    return block;
  }
}

static ViviCodeValue *
vivi_parser_function_call_new (ViviCodeValue *name)
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
vivi_parser_assignment_new (ViviCodeValue *left, ViviCodeValue *right)
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
vivi_parser_get_new (ViviCodeValue *from, ViviCodeValue *name)
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
vivi_parser_value_is_left_hand_side (ViviCodeValue *value)
{
  // FIXME: Correct?
  return VIVI_IS_CODE_GET (value);
}

static gboolean
vivi_parser_value_is_identifier (ViviCodeValue *value)
{
  if (!VIVI_IS_CODE_GET (value))
    return FALSE;
  return VIVI_IS_CODE_CONSTANT (VIVI_CODE_GET (value)->name);
}

static void
vivi_parser_start_level (ParseData *data)
{
  g_return_if_fail (data != NULL);

  if (data->level != NULL)
    data->levels = g_slist_prepend (data->levels, data->level);
  data->level = g_new0 (ParseLevel, 1);
}

static ViviCodeLabel *
vivi_parser_find_label (ParseData *data, const char *name)
{
  GSList *iter;

  for (iter = data->level->labels; iter != NULL; iter = iter->next) {
    if (g_str_equal (vivi_code_label_get_name (VIVI_CODE_LABEL (iter->data)),
	  name))
      return VIVI_CODE_LABEL (iter->data);
  }

  return NULL;
}

static void
vivi_parser_end_level (ParseData *data)
{
  GSList *iter;

  g_return_if_fail (data != NULL);
  g_return_if_fail (data->level != NULL);

  for (iter = data->level->gotos; iter != NULL; iter = iter->next) {
    ViviCompilerGotoName *goto_;
    ViviCodeLabel *label;

    goto_ = VIVI_COMPILER_GOTO_NAME (iter->data);
    label = vivi_parser_find_label (data,
	vivi_compiler_goto_name_get_name (goto_));

    if (label != NULL) {
      vivi_compiler_goto_name_set_label (goto_, label);
    } else {
      vivi_parser_error (data, "Label named '%s' doesn't exist in this block",
	  vivi_compiler_goto_name_get_name (goto_));
    }

    g_object_unref (goto_);
  }
  g_slist_free (data->level->gotos);

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

static void
vivi_parser_add_goto (ParseData *data, ViviCompilerGotoName *goto_)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (data->level != NULL);
  g_return_if_fail (VIVI_IS_COMPILER_GOTO_NAME (goto_));

  data->level->gotos =
    g_slist_prepend (data->level->gotos, g_object_ref (goto_));
}

static gboolean
vivi_parser_add_label (ParseData *data, ViviCodeLabel *label)
{
  GSList *iter;

  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (data->level != NULL, FALSE);
  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), FALSE);

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

/* ActionScript specific */
static gboolean
parse_undefined_literal (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_UNDEFINED)) {
    vivi_parser_cancel_unexpected (data, TOKEN_UNDEFINED);
    return FALSE;
  }

  *value = vivi_code_constant_new_undefined ();
  return TRUE;
}

static gboolean
parse_null_literal (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_NULL)) {
    vivi_parser_cancel_unexpected (data, TOKEN_NULL);
    return FALSE;
  }

  *value = vivi_code_constant_new_null ();
  return TRUE;
}

static gboolean
parse_boolean_literal (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_BOOLEAN)) {
    vivi_parser_cancel_unexpected (data, TOKEN_BOOLEAN);
    return FALSE;
  }

  *value = vivi_code_constant_new_boolean (data->scanner->value.v_boolean);
  return TRUE;
}

static gboolean
parse_numeric_literal (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_NUMBER)) {
    vivi_parser_cancel_unexpected (data, TOKEN_NUMBER);
    return FALSE;
  }

  *value = vivi_code_constant_new_number (data->scanner->value.v_number);
  return TRUE;
}

static gboolean
parse_string_literal (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_STRING)) {
    vivi_parser_cancel_unexpected (data, TOKEN_STRING);
    return FALSE;
  }

  *value = vivi_code_constant_new_string (data->scanner->value.v_string);
  return TRUE;
}

static gboolean
parse_literal (ParseData *data, ViviCodeValue **value)
{
  static const ParseValueFunction functions[] = {
    parse_undefined_literal,
    parse_null_literal,
    parse_boolean_literal,
    parse_numeric_literal,
    parse_string_literal,
    NULL
  };
  guint i;

  *value = NULL;

  for (i = 0; functions[i] != NULL; i++) {
    if (functions[i] (data, value))
      return TRUE;
  }

  vivi_parser_cancel_unexpected (data, ERROR_TOKEN_LITERAL);
  return FALSE;
}

static gboolean
parse_identifier (ParseData *data, ViviCodeValue **value)
{
  *value = NULL;

  if (!check_token (data, TOKEN_IDENTIFIER)) {
    vivi_parser_cancel_unexpected (data, TOKEN_IDENTIFIER);
    return FALSE;
  }

  *value = vivi_code_get_new_name (data->scanner->value.v_identifier);

  return TRUE;
}

static gboolean
parse_property_name (ParseData *data, ViviCodeValue **value)
{
  static const ParseValueFunction functions[] = {
    parse_identifier,
    parse_string_literal,
    parse_numeric_literal,
    NULL
  };
  guint i;

  *value = NULL;

  for (i = 0; functions[i] != NULL; i++) {
    if (functions[i] (data, value))
      return TRUE;
  }

  vivi_parser_cancel_unexpected (data, ERROR_TOKEN_PROPERTY_NAME);
  return FALSE;
}

static gboolean
parse_assignment_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement);

static gboolean
parse_array_literal (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *member;
  ViviCodeStatement *statement_new;

  *value = NULL;
  *statement = NULL;

  if (!check_token (data, TOKEN_BRACKET_LEFT)) {
    vivi_parser_cancel_unexpected (data, TOKEN_BRACKET_LEFT);
    return FALSE;
  }

  *value = vivi_code_init_array_new ();

  while (TRUE) {
    if (check_token (data, TOKEN_BRACKET_RIGHT)) {
      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (*value),
	 vivi_code_constant_new_undefined ());
      break;
    } else if (check_token (data, TOKEN_COMMA)) {
      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (*value),
	 vivi_code_constant_new_undefined ());
    } else if (parse_assignment_expression (data, &member, &statement_new)) {
      *statement = vivi_parser_join_statements (*statement, statement_new);

      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (*value),
	  member);
      g_object_unref (member);

      if (!check_token (data, TOKEN_COMMA)) {
	if (!check_token (data, TOKEN_BRACKET_RIGHT)) {
	  vivi_parser_error_unexpected (data, TOKEN_BRACKET_RIGHT,
	      TOKEN_COMMA);
	}
	break;
      }
    } else {
      vivi_parser_error_unexpected (data, TOKEN_BRACKET_RIGHT, TOKEN_COMMA,
	  ERROR_TOKEN_ASSIGNMENT_EXPRESSION);
      break;
    }
  }

  return TRUE;
}

static gboolean
parse_object_literal (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  gboolean status;

  *value = NULL;
  *statement = NULL;

  if (!check_token (data, TOKEN_BRACE_LEFT)) {
    vivi_parser_cancel_unexpected (data, TOKEN_BRACE_LEFT);
    return FALSE;
  }

  *value = vivi_code_init_object_new ();

  if (!check_token (data, TOKEN_BRACE_RIGHT)) {
    do {
      ViviCodeValue *property, *initializer;
      ViviCodeStatement *statement_new;

      if (!parse_property_name (data, &property))
	vivi_parser_error_child_value (data, &property);

      if (!check_token (data, TOKEN_COLON))
	vivi_parser_error_unexpected (data, TOKEN_COLON);

      if (!parse_assignment_expression (data, &initializer, &statement_new))
	vivi_parser_error_child_value (data, &initializer);

      *statement = vivi_parser_join_statements (*statement, statement_new);

      vivi_code_init_object_add_variable (VIVI_CODE_INIT_OBJECT (*value),
	  property, initializer);
    } while (check_token (data, TOKEN_COMMA));
  }

  if (!check_token (data, TOKEN_BRACE_RIGHT))
    vivi_parser_error_unexpected (data, TOKEN_BRACE_RIGHT);

  return TRUE;
}

// misc

static gboolean
parse_variable_declaration (ParseData *data, ViviCodeStatement **statement)
{
  gboolean status;
  ViviCodeValue *identifier, *value;
  ViviCodeStatement *assignment, *statement_right;

  *statement = NULL;

  if (!parse_identifier (data, &identifier)) {
    vivi_parser_cancel_unexpected (data, ERROR_TOKEN_IDENTIFIER);
    return FALSE;
  }

  if (check_token (data, TOKEN_ASSIGN)) {
    if (!parse_assignment_expression (data, &value, &statement_right))
      vivi_parser_error_child_value (data, value);
  } else {
    value = vivi_code_constant_new_undefined ();
    statement_right = NULL;
  }

  assignment = vivi_parser_assignment_new (identifier, value);
  vivi_code_assignment_set_local (VIVI_CODE_ASSIGNMENT (assignment), TRUE);

  *statement = vivi_parser_join_statements (statement_right, assignment);

  return TRUE;
}

// expression

static gboolean
parse_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement);

static gboolean
parse_primary_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  *value = NULL;
  *statement = NULL;

  if (check_token (data, TOKEN_THIS)) {
    *value = vivi_code_get_new_name ("this");
    return TRUE;
  }

  if (check_token (data, TOKEN_PARENTHESIS_LEFT)) {
    if (!parse_expression (data, value, statement))
      vivi_parser_error_child_value (data, value);
    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT))
      vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_RIGHT);
    return TRUE;
  }

  if (parse_object_literal (data, value, statement))
    return TRUE;

  if (parse_array_literal (data, value, statement))
    return TRUE;

  if (parse_identifier (data, value))
    return TRUE;

  if (parse_literal (data, value))
    return TRUE;

  vivi_parser_cancel_unexpected (ERROR_TOKEN_PRIMARY_EXPRESSION);
  return FALSE;
}

static gboolean
parse_function_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement);

static gboolean
parse_member_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  gboolean status;
  ViviCodeValue *member;
  ViviCodeStatement *statement_member;

  *value = NULL;
  *statement = NULL;

  // TODO: new MemberExpression Arguments

  if (!parse_primary_expression (data, value, statement)) {
    if (!parse_function_expression (data, value, statement)) {
      vivi_parser_cancel_unexpected (data, ERROR_PRIMARY_EXPRESSION,
	  ERROR_FUNCTION_EXPRESSION);
      return FALSE;
    }
  }

   while (TRUE) {
    ViviCodeValue *tmp;

    if (check_token (data, TOKEN_BRACKET_LEFT)) {
      if (!parse_expression (data, &member, &statement_member))
	vivi_parser_error_child_value (data, &member);

      *statement = vivi_parser_join_statements (*statement, statement_member);

      if (!check_token (data, TOKEN_BRACKET_RIGHT))
	vivi_parser_error_unexpected (data, TOKEN_BRACKET_RIGHT);
    } else if (check_token (data, TOKEN_DOT)) {
      if (!parse_identifier (data, &member))
	vivi_parser_error_child_value (data, &member);
    } else {
      return TRUE;
    }

    tmp = *value;
    *value = vivi_parser_get_new (tmp, member);
    g_object_unref (tmp);
    g_object_unref (member);
  }

  g_assert_not_reached ();
}

static gboolean
parse_new_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  gboolean status;

  *value = NULL;
  *statement = NULL;

  if (check_token (data, TOKEN_NEW)) {
    if (!parse_new_expression (data, value, statement))
      vivi_parser_error_child_value (data, value);
    if (!VIVI_IS_CODE_FUNCTION_CALL (*value)) {
      ViviCodeValue *tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_parser_function_call_new (tmp);
      g_object_unref (tmp);
    }
    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (*value),
	TRUE);
    return TRUE;
  } else {
    return parse_member_expression (data, value, statement);
  }
}

static gboolean
parse_left_hand_side_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *name;
  ViviCodeValue **arguments;
  ViviCodeStatement *argument_statement;
  guint i;

  *value = NULL;
  *statement = NULL;

  if (check_token (data, TOKEN_NEW)) {
    if (!parse_new_expression (data, value, statement))
      vivi_parser_error_child_value (data, value);
    if (!VIVI_IS_CODE_FUNCTION_CALL (*value)) {
      ViviCodeValue *tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_parser_function_call_new (tmp);
      g_object_unref (tmp);
    }
    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (*value),
	TRUE);
    return TRUE;
  }

  if (!parse_member_expression (data, value, statement)) {
    vivi_parser_cancel_unexpected (data, ERROR_TOKEN_MEMBER_EXPRESSION);
    return FALSE;
  }

  while (TRUE) {
    // TODO: member expressions?
    if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
      break;

    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      if (!parse_value_statement_list (data, parse_assignment_expression,
	    &arguments, &argument_statement, TOKEN_COMMA)) {
	// TODO
	arguments = NULL;
      }

      *statement =
	vivi_parser_join_statements (*statement, argument_statement);

      if (!check_token (data, TOKEN_PARENTHESIS_RIGHT))
	vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_RIGHT);
    } else {
      arguments = NULL;
    }

    name = *value;
    *value = vivi_parser_function_call_new (name);
    g_object_unref (name);

    if (arguments != NULL) {
      for (i = 0; arguments[i] != NULL; i++) {
	vivi_code_function_call_add_argument (VIVI_CODE_FUNCTION_CALL (*value),
	    arguments[i]);
      }
      free_value_list (arguments);
    }
  }

  return TRUE;
}

static gboolean
parse_postfix_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *operation, *one, *temporary;
  const char *operator;

  *value = NULL;
  *statement = NULL;

  if (!parse_left_hand_side_expression (data, value, statement)) {
    vivi_parser_cancel_unexpected (data, ERROR_TOKEN_POSTFIX_EXPRESSION);
    return FALSE;
  }

  if (check_line_terminator (data))
    return TRUE;

  if (check_token (data, TOKEN_INCREASE)) {
    operator = "+";
  } else if (check_token (data, TOKEN_DESCREASE)) {
    operator = "-";
  } else {
    return TRUE;
  }

  // FIXME: Correct?
  if (!vivi_parser_value_is_left_hand_side (*value)) {
    vivi_parser_error (
	"Invalid left-hand side expression for INCREASE/DECREASE");
  }

  one = vivi_code_constant_new_number (1);
  operation = vivi_code_binary_new_name (*value, one, operator);
  g_object_unref (one);

  temporary = vivi_compiler_get_temporary_new ();
  *statement = vivi_parser_join_statements (*statement,
      vivi_parser_join_statements (
	vivi_parser_assignment_new (temporary, *value),
	vivi_parser_assignment_new (*value, operation)));
  g_object_unref (operation);

  g_object_unref (*value);
  *value = temporary;

  return TRUE;
}

static gboolean
parse_unary_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  gboolean status;
  ViviCodeValue *tmp, *one;
  const char *operator;

  *value = NULL;

  operator = NULL;

  vivi_parser_scanner_peek_next_token (data->scanner);
  switch ((guint)data->scanner->next_token) {
    /*case TOKEN_DELETE:
    case TOKEN_VOID:
    case TOKEN_TYPEOF:*/
    case TOKEN_INCREASE:
      operator = "+";
      // fall through
    case TOKEN_DESCREASE:
      if (!operator) operator = "-";

      vivi_parser_scanner_get_next_token (data->scanner);

      if (!parse_unary_expression (data, value, statement))
	vivi_parser_error_child_value (data, value);

      // FIXME: Correct?
      if (!vivi_parser_value_is_left_hand_side (*value)) {
	vivi_parser_error (
	    "Invalid left-hand side expression for INCREASE/DECREASE");
      }

      one = vivi_code_constant_new_number (1);
      tmp = vivi_code_binary_new_name (*value, one, operator);
      g_object_unref (one);

      *statement = vivi_parser_join_statements (*statement,
	  vivi_parser_assignment_new (*value, tmp));
      g_object_unref (tmp);

      return TRUE;
    /*case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BITWISE_NOT:*/
    case TOKEN_LOGICAL_NOT:
      vivi_parser_scanner_get_next_token (data->scanner);

      if (!parse_unary_expression (data, value, statement))
	vivi_parser_error_child_value (data, value);

      tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_code_unary_new (tmp, '!');
      g_object_unref (tmp);

      return TRUE;
    default:
      return parse_postfix_expression (data, value, statement);
  }
}

typedef enum {
  PASS_ALWAYS,
  PASS_LOGICAL_OR,
  PASS_LOGICAL_AND
} ParseOperatorPass;

static gboolean
parse_operator_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement, const ViviParserScannerToken *tokens,
    ParseOperatorPass pass, ParseValueStatementFunction next_parse_function)
{
  ViviCodeValue *left, *right;
  ViviCodeStatement *statement_right;
  guint i;

  *value = NULL;
  *statement = NULL;

  if (!next_parse_function (data, value, statement)) {
    // FIXME: token?
    return FALSE;
  }

  for (i = 0; tokens[i] != TRUE; i++) {
    while (check_token (data, tokens[i])) {
      if (!next_parse_function (data, &right, &statement_right))
	vivi_parser_error_child_value (data, &right);

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

	*statement = vivi_parser_join_statements (*statement, statement_right);
      }

      left = VIVI_CODE_VALUE (*value);
      *value = vivi_code_binary_new_name (left, VIVI_CODE_VALUE (right),
	  vivi_parser_scanner_token_name (tokens[i]));
      g_object_unref (left);
      g_object_unref (right);
    };
  }

  return TRUE;
}

static gboolean
parse_multiplicative_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_MULTIPLY,
    TOKEN_DIVIDE, TOKEN_REMAINDER, TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_unary_expression);
}

static gboolean
parse_additive_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_multiplicative_expression);
}

static gboolean
parse_shift_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT, TOKEN_SHIFT_RIGHT_UNSIGNED, TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_additive_expression);
}

static gboolean
parse_relational_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN, /*TOKEN_LESS_THAN_OR_EQUAL,
    TOKEN_EQUAL_OR_GREATER_THAN, TOKEN_INSTANCEOF, TOKEN_IN,*/ TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_shift_expression);
}

static gboolean
parse_equality_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_EQUAL,
    /*TOKEN_NOT_EQUAL,*/ TOKEN_STRICT_EQUAL, /*TOKEN_NOT_STRICT_EQUAL,*/
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_relational_expression);
}

static gboolean
parse_bitwise_and_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_AND,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_equality_expression);
}

static gboolean
parse_bitwise_xor_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_XOR,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_bitwise_and_expression);
}

static gboolean
parse_bitwise_or_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_OR,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_ALWAYS, parse_bitwise_xor_expression);
}

static gboolean
parse_logical_and_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LOGICAL_AND,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_LOGICAL_AND, parse_bitwise_or_expression);
}

static gboolean
parse_logical_or_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LOGICAL_OR,
    TOKEN_NONE };

  return parse_operator_expression (data, value, statement, tokens,
      PASS_LOGICAL_OR, parse_logical_and_expression);
}

static gboolean
parse_conditional_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  //ViviCodeStatement *if_statement, *else_statement;

  *value = NULL;

  if (!parse_logical_or_expression (data, value, statement))
    return FALSE;

#if 0
  if (!check_token (data, TOKEN_QUESTION_MARK)) {
    *statement = vivi_code_value_statement_new (VIVI_CODE_VALUE (value));
    g_object_unref (value);
    return TRUE;
  }

  status = parse_assignment_expression (data, &if_statement);
  if (status != TRUE) {
    g_object_unref (value);
    return FAIL_CHILD (status);
  }

  if (!check_token (data, TOKEN_COLON)) {
    g_object_unref (value);
    g_object_unref (if_statement);
    return TOKEN_COLON;
  }

  status = parse_assignment_expression (data, &else_statement);
  if (status != TRUE) {
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

  return TRUE;
}

static gboolean
parse_assignment_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *right;
  ViviCodeStatement *assignment, *statement_right;
  const char *operator;

  *value = NULL;
  *statement = NULL;

  if (!parse_conditional_expression (data, value, statement)) {
    vivi_parser_cancel_unexpected (data, ERROR_TOKEN_CONDITIONAL_EXPRESSION);
    return FALSE;
  }

  if (!vivi_parser_value_is_left_hand_side (*value))
    return TRUE;

  operator = NULL;

  vivi_parser_scanner_peek_next_token (data->scanner);
  switch ((guint)data->scanner->next_token) {
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
      vivi_parser_scanner_get_next_token (data->scanner);

      if (parse_assignment_expression (data, &right, &statement_right))
	vivi_parser_error_child_value (data, &right);

      if (operator != NULL) {
	assignment = vivi_parser_assignment_new (*value,
	    vivi_code_binary_new_name (*value, right, operator));
      } else {
	assignment = vivi_parser_assignment_new (*value, right);
      }
      g_object_unref (right);

      *statement = vivi_parser_join_statements (*statement,
	  vivi_parser_join_statements (statement_right, assignment));

      return TRUE;
    default:
      return TRUE;
  }

  g_assert_not_reached ();
}

static gboolean
parse_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeStatement *statement_one;

  *statement = NULL;

  if (parse_assignment_expression (data, value, &statement_one)) {
    vivi_parser_cancel_unexpected (data, ERROR_TOKEN_ASSIGNMENT_EXPRESSION);
    return FALSE;
  }

  while (TRUE) {
    *statement = vivi_parser_join_statements (*statement, statement_one);

    if (!check_token (data, TOKEN_COMMA))
      break;

    g_object_unref (*value);

    if (!parse_assignment_expression (data, value, &statement_one))
	vivi_parser_error_child_value (data, value);
  }

  return TRUE;
}

// statement

static gboolean
parse_statement (ParseData *data, ViviCodeStatement **statement);

static gboolean
parse_trace_statement (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value;
  ViviCodeStatement *expression_statement;

  *statement = NULL;

  if (!check_token (data, TOKEN_TRACE)) {
    vivi_parser_cancel_unexpected (data, TOKEN_TRACE);
    return FALSE;
  }

  if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
    vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_LEFT);

  if (parse_expression (data, &value, &expression_statement))
    vivi_parser_error_child_value (data, &value);

  if (!check_token (data, TOKEN_PARENTHESIS_RIGHT))
    vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_RIGHT);

  if (!check_automatic_semicolon (data))
    vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);

  *statement = vivi_code_trace_new (value);
  g_object_unref (value);

  *statement = vivi_parser_join_statements (expression_statement, *statement);

  return TRUE;
}

static gboolean
parse_continue_or_break_statement (ParseData *data,
    ViviCodeStatement **statement, ViviParserScannerToken token)
{
  *statement = NULL;

  if (!check_token (data, token)) {
    vivi_parser_cancel_unexpected (data, token);
    return FALSE;
  }

  if (!check_restricted_semicolon (data)) {
    gboolean status;
    ViviCodeValue *identifier;

    if (!parse_identifier (data, &identifier))
      vivi_parser_error_child_value (data, &identifier);

    // FIXME
    *statement = vivi_compiler_goto_name_new (
	vivi_code_constant_get_variable_name (
	  VIVI_CODE_CONSTANT (VIVI_CODE_GET (identifier)->name)));
    g_object_unref (identifier);

    vivi_parser_add_goto (data, VIVI_COMPILER_GOTO_NAME (*statement));

    if (!check_automatic_semicolon (data))
      vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);
  } else {
    *statement = vivi_code_break_new ();
  }

  return TRUE;
}

static gboolean
parse_continue_statement (ParseData *data, ViviCodeStatement **statement)
{
  return parse_continue_or_break_statement (data, statement, TOKEN_CONTINUE);
}

static gboolean
parse_break_statement (ParseData *data, ViviCodeStatement **statement)
{
  return parse_continue_or_break_statement (data, statement, TOKEN_BREAK);
}

static gboolean
parse_throw_statement (ParseData *data, ViviCodeStatement **statement)
{
  gboolean status;
  ViviCodeValue *value;
  ViviCodeStatement *expression_statement;

  *statement = NULL;

  if (!check_token (data, TOKEN_THROW)) {
    vivi_parser_cancel_unexpected (data, TOKEN_THROW);
    return FALSE;
  }

  if (check_line_terminator (data))
    vivi_parser_error_unexpected_line_terminator (data,
	ERROR_TOKEN_EXPRESSION);

  if (parse_expression (data, &value, &expression_statement))
    vivi_parser_error_child_value (data, &value);

  *statement = vivi_code_throw_new (value);
  g_object_unref (value);

  *statement = vivi_parser_join_statements (expression_statement, *statement);

  if (!check_automatic_semicolon (data))
    vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);

  return TRUE;
}

static gboolean
parse_return_statement (ParseData *data, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (data, TOKEN_RETURN)) {
    vivi_parser_cancel_unexpected (data, TOKEN_RETURN);
    return FALSE;
  }

  *statement = vivi_code_return_new ();

  if (!check_restricted_semicolon (data)) {
    gboolean status;
    ViviCodeValue *value;
    ViviCodeStatement *expression_statement;

    if (!parse_expression (data, &value, &expression_statement)) {
      vivi_parser_error_unexpected (data, TOKEN_SEMICOLON,
	  ERROR_TOKEN_EXPRESSION);
      return TRUE;
    }

    vivi_code_return_set_value (VIVI_CODE_RETURN (*statement), value);
    g_object_unref (value);

    *statement =
      vivi_parser_join_statements (expression_statement, *statement);

    if (!check_automatic_semicolon (data))
      vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);
  }

  return TRUE;
}

static gboolean
parse_iteration_statement (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *condition;
  ViviCodeStatement *pre_statement, *condition_statement, *loop_statement;

  *statement = NULL;

  pre_statement = NULL;
  condition_statement = NULL;

  if (check_token (data, TOKEN_DO)) {
    if (!parse_statement (data, &loop_statement))
      vivi_parser_error_child_statement (data, &loop_statement);

    if (!check_token (data, TOKEN_WHILE))
      vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);

    if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
      vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_LEFT);

    if (parse_expression (data, &condition, &condition_statement))
      vivi_parser_error_child_value (data, &condition);

    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT))
      vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_RIGHT);

    if (!check_automatic_semicolon (data))
      vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);

    pre_statement = g_object_ref (loop_statement);
  } else if (check_token (data, TOKEN_WHILE)) {
    if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
      vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_LEFT);

    if (parse_expression (data, &condition, &condition_statement))
      vivi_parser_error_child_value (data, &condition);

    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT))
      vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_RIGHT);

    if (!parse_statement (data, &loop_statement))
      vivi_parser_error_child_statement (data, &loop_statement);
  } else if (check_token (data, TOKEN_FOR)) {
    ViviCodeValue *pre_value;
    ViviCodeStatement *post_statement;

    if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
      vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_LEFT);

    if (check_token (data, TOKEN_VAR)) {
      // FIXME: no in
      if (parse_statement_list (data, parse_variable_declaration,
	    &pre_statement, TOKEN_COMMA)) {
	// FIXME
      }
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
	if (parse_expression (data, &pre_value, &pre_statement))
	  vivi_parser_error_child_value (data, &pre_value);
      } else {
	pre_value = NULL;
	pre_statement = NULL;
      }
    }

    if (check_token (data, TOKEN_SEMICOLON)) {
      if (pre_value != NULL)
	g_object_unref (pre_value);
      if (!check_token (data, TOKEN_SEMICOLON)) {
	if (parse_expression (data, &condition, &condition_statement))
	  vivi_parser_error_child_value (data, &condition);

	if (!check_token (data, TOKEN_SEMICOLON))
	  vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);
      }

      if (!parse_expression (data, &pre_value, &post_statement))
	vivi_parser_error_child_value (data, &pre_value);
      g_object_unref (pre_value);
    } else if (pre_value != NULL && check_token (data, TOKEN_IN)) {
      post_statement = NULL;

      // FIXME: correct?
      if (!vivi_parser_value_is_left_hand_side (pre_value))
	vivi_parser_error ("Invalid left-hand side expression for in");

      g_object_unref (pre_value);
      if (pre_statement != NULL)
	g_object_unref (pre_statement);
      vivi_parser_error ("for (... in ...) has not been implemented yet");
    } else {
      if (pre_value != NULL) {
	vivi_parser_error_unexpected (data, TOKEN_SEMICOLON, TOKEN_IN);
      } else {
	vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);
      }

      condition = vivi_code_constant_new_undefined ();

      if (pre_value != NULL)
	g_object_unref (pre_value);
      if (pre_statement != NULL)
	g_object_unref (pre_statement);
    }

    if (!check_token (data, TOKEN_PARENTHESIS_RIGHT))
      vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_RIGHT);

    if (!parse_statement (data, &loop_statement))
      vivi_parser_error_child_statement (data, &loop_statement);

    loop_statement =
      vivi_parser_join_statements (loop_statement, post_statement);
  } else {
    vivi_parser_cancel_unexpected (data, ERROR_TOKEN_ITERATION_STATEMENT);
    return FALSE;
  }

  if (condition_statement != NULL) {
    pre_statement = vivi_parser_join_statements (pre_statement,
	g_object_ref (condition_statement));
    loop_statement = vivi_parser_join_statements (loop_statement,
	g_object_ref (condition_statement));
    g_object_unref (condition_statement);
  }

  *statement = vivi_code_loop_new ();
  vivi_code_loop_set_condition (VIVI_CODE_LOOP (*statement), condition);
  g_object_unref (condition);
  vivi_code_loop_set_statement (VIVI_CODE_LOOP (*statement), loop_statement);
  g_object_unref (loop_statement);

  *statement = vivi_parser_join_statements (pre_statement, *statement);

  return TRUE;
}

static gboolean
parse_if_statement (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *condition;
  ViviCodeStatement *pre_statement, *if_statement, *else_statement;

  *statement = NULL;

  if (!check_token (data, TOKEN_IF)) {
    vivi_parser_cancel_unexpected (data, TOKEN_IF);
    return FALSE;
  }

  if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
    vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_LEFT);

  if (!parse_expression (data, &condition, &pre_statement))
    vivi_parser_error_child_value (data, &condition);

  if (!check_token (data, TOKEN_PARENTHESIS_RIGHT))
    vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_RIGHT);

  if (!parse_statement (data, &if_statement))
    vivi_parser_error_child_statement (data, &if_statement);

  if (check_token (data, TOKEN_ELSE)) {
    if (!parse_statement (data, &else_statement))
      vivi_parser_error_child_statement (data, &else_statement);
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

  *statement = vivi_parser_join_statements (pre_statement, *statement);

  g_assert (*statement != NULL);

  return TRUE;
}

static gboolean
parse_expression_statement (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value;
  ViviCodeStatement *last;

  *statement = NULL;

  vivi_parser_scanner_peek_next_token (data->scanner);
  if (data->scanner->next_token == TOKEN_BRACE_LEFT ||
      data->scanner->next_token == TOKEN_FUNCTION) {
    vivi_parser_cancel_unexpected (data, ERROR_TOKEN_EXPRESSION_STATEMENT);
    return FALSE;
  }

  if (!parse_expression (data, &value, statement))
    return FALSE;

  // check for label
  if (*statement == NULL && vivi_parser_value_is_identifier (value)) {
    if (check_token (data, TOKEN_COLON)) {
      *statement = vivi_code_label_new (vivi_code_constant_get_variable_name (
	    VIVI_CODE_CONSTANT (VIVI_CODE_GET (value)->name)));
      if (!vivi_parser_add_label (data, VIVI_CODE_LABEL (*statement)))
	vivi_parser_error ("Same label name used twice");
      return TRUE;
    }
  }

  if (!check_automatic_semicolon (data))
    vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);

  // add a value statement, if the last statement is not an assignment with the
  // same value
  if (VIVI_IS_CODE_BLOCK (*statement)) {
    ViviCodeBlock *block = VIVI_CODE_BLOCK (*statement);

    last = vivi_code_block_get_statement (block,
	vivi_code_block_get_n_statements (block) - 1);
  } else {
    last = *statement;
  }

  if (VIVI_IS_CODE_ASSIGNMENT (last) && VIVI_IS_CODE_GET (value)) {
    ViviCodeAssignment *assignment = VIVI_CODE_ASSIGNMENT (last);

    if (assignment->from == NULL && assignment->name == VIVI_CODE_GET (value)->name) {
      g_object_unref (value);
      return TRUE;
    }
  }

  *statement = vivi_parser_join_statements (*statement,
      vivi_code_value_statement_new (value));
  g_object_unref (value);

  return TRUE;
}

static gboolean
parse_empty_statement (ParseData *data, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!check_token (data, TOKEN_SEMICOLON)) {
    vivi_parser_cancel_unexpected (data, TOKEN_SEMICOLON);
    return FALSE;
  }

  *statement = vivi_compiler_empty_statement_new ();

  return TRUE;
}

static gboolean
parse_block (ParseData *data, ViviCodeStatement **statement)
{
  gboolean status;

  *statement = NULL;

  if (!check_token (data, TOKEN_BRACE_LEFT)) {
    vivi_parser_cancel_unexpected (data, TOKEN_BRACE_LEFT);
    return FALSE;
  }

  vivi_parser_scanner_peek_next_token (data->scanner);
  if (!check_token (data, TOKEN_BRACE_RIGHT)) {
    if (!parse_statement_list (data, parse_statement, statement, TRUE)) {
      // FIXME: child token
      vivi_parser_error_unexpected (data, TOKEN_BRACE_RIGHT,
	  ERROR_TOKEN_STATEMENT);
      *statement = vivi_code_block_new ();
    }

    if (!check_token (data, TOKEN_BRACE_RIGHT))
      vivi_parser_error_unexpected (data, TOKEN_BRACE_RIGHT);
  } else {
    *statement = vivi_code_block_new ();
  }

  return TRUE;
}

static gboolean
parse_variable_statement (ParseData *data, ViviCodeStatement **statement)
{
  gboolean status;

  *statement = NULL;

  if (!check_token (data, TOKEN_VAR)) {
    vivi_parser_cancel_unexpected (data, TOKEN_VAR);
    return FALSE;
  }

  if (!parse_statement_list (data, parse_variable_declaration, statement,
	TOKEN_COMMA)) {
    // FIXME
  }

  if (!check_automatic_semicolon (data))
    vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);

  return TRUE;
}

static gboolean
parse_statement (ParseData *data, ViviCodeStatement **statement)
{
  static const ParseStatementFunction functions[] = {
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
    parse_throw_statement,
    //parse_try_statement,
    parse_trace_statement,
    NULL
  };
  guint i;

  *statement = NULL;

  for (i = 0; functions[i] != NULL; i++) {
    if (functions[i] (data, statement))
      return TRUE;
  }

  vivi_parser_cancel_unexpected (data, ERROR_TOKEN_STATEMENT);
  return FALSE;
}

// function

static gboolean
parse_source_element (ParseData *data, ViviCodeStatement **statement);

static gboolean
parse_function_definition (ParseData *data, ViviCodeValue **function,
    ViviCodeValue **identifier, gboolean identifier_required)
{
  ViviCodeValue **arguments;
  ViviCodeStatement *body;
  guint i;

  *function = NULL;
  *identifier = NULL;

  arguments = NULL;
  body = NULL;

  if (!check_token (data, TOKEN_FUNCTION)) {
    vivi_parser_cancel_unexpected (data, TOKEN_FUNCTION);
    return FALSE;
  }

  if (identifier_required) {
    if (!parse_identifier (data, identifier))
      vivi_parser_error_child_value (data, identifier);

    if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
      vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_LEFT);
  } else {
    if (!check_token (data, TOKEN_PARENTHESIS_LEFT)) {
      if (!parse_identifier (data, identifier)) {
	// FIXME: child token
	vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_LEFT,
	    ERROR_TOKEN_IDENTIFIER);
      }

      if (!check_token (data, TOKEN_PARENTHESIS_LEFT))
	vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_LEFT);
    }
  }

  if (!parse_value_list (data, parse_identifier, &arguments, TOKEN_COMMA)) {
    // FIXME
  }

  if (!check_token (data, TOKEN_PARENTHESIS_RIGHT))
    vivi_parser_error_unexpected (data, TOKEN_PARENTHESIS_RIGHT);

  if (!check_token (data, TOKEN_BRACE_LEFT))
    vivi_parser_error_unexpected (data, TOKEN_BRACE_LEFT);

  vivi_parser_start_level (data);

  if (parse_statement_list (data, parse_source_element, &body, TRUE)) {
    // FIXME
  }

  vivi_parser_end_level (data);

  if (!check_token (data, TOKEN_BRACE_RIGHT))
    vivi_parser_error_unexpected (data, TOKEN_BRACE_RIGHT);

  *function = vivi_code_function_new ();
  if (body != NULL) {
    vivi_code_function_set_body (VIVI_CODE_FUNCTION (*function), body);
    g_object_unref (body);
  }
  if (arguments != NULL) {
    for (i = 0; arguments[i] != NULL; i++) {
      vivi_code_function_add_argument (VIVI_CODE_FUNCTION (*function),
	  vivi_code_constant_get_variable_name (VIVI_CODE_CONSTANT (
	      VIVI_CODE_GET (arguments[i])->name)));
    }
    free_value_list (arguments);
  }

  return TRUE;
}

static gboolean
parse_function_declaration (ParseData *data, ViviCodeStatement **statement)
{
  gboolean status;
  ViviCodeValue *function, *identifier;

  *statement = NULL;

  if (!parse_function_definition (data, &function, &identifier, TRUE))
    return FALSE;

  *statement = vivi_parser_assignment_new (identifier, function);
  g_object_unref (identifier);
  g_object_unref (function);

  return TRUE;
}

static gboolean
parse_function_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  gboolean status;
  ViviCodeValue *identifier;

  *statement = NULL;

  if (!parse_function_definition (data, &function, &identifier, FALSE))
    return FALSE;

  if (identifier != NULL) {
    *statement = vivi_parser_assignment_new (identifier, *value);
    g_object_unref (identifier);
  }

  return TRUE;
}

// top

static gboolean
parse_source_element (ParseData *data, ViviCodeStatement **statement)
{
  *statement = NULL;

  if (!parse_function_declaration (data, statement)) {
    if (!parse_statement (data, statement)) {
      // FIXME
      vivi_parser_cancel_unexpected (data, ERROR_TOKEN_FUNCTION_DECLARATION,
	  ERROR_TOKEN_STATEMENT);
      return FALSE;
    }
  }

  return TRUE;
}

static void
parse_program (ParseData *data, ViviCodeStatement **statement)
{
  g_assert (data->level == NULL);
  vivi_parser_start_level (data);

  *statement = NULL;

  if (!parse_statement_list (data, parse_source_element, statement, TRUE))
    // FIXME

  if (!check_token (data, TOKEN_EOF))
    vivi_parser_error_unexpected (data, TOKEN_EOF);

  vivi_parser_end_level (data);

  g_assert (data->level == NULL);
}

// parsing

static gboolean
parse_statement_list (ParseData *data, ParseStatementFunction function,
    ViviCodeStatement **block, guint separator)
{
  ViviCodeStatement *statement;
  gboolean status;

  g_assert (data != NULL);
  g_assert (function != NULL);
  g_assert (block != NULL);

  *block = NULL;

  status = function (data, &statement);
  if (status != TRUE)
    return status;

  *block = vivi_code_block_new ();

  do {
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (*block), statement);
    g_object_unref (statement);

    if (separator != TRUE && !check_token (data, separator))
      break;

    status = function (data, &statement);
    if (status == STATUS_FAIL) {
      g_object_unref (*block);
      *block = NULL;
      return STATUS_FAIL;
    }
  } while (status == TRUE);

  return TRUE;
}

static gboolean
parse_value_statement_list (ParseData *data,
    ParseValueStatementFunction function, ViviCodeValue ***list,
    ViviCodeStatement **statement, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;
  ViviCodeStatement *statement_one;
  gboolean status;

  g_assert (data != NULL);
  g_assert (function != NULL);
  g_assert (list != NULL);
  g_assert (statement != NULL);

  *list = NULL;
  *statement = NULL;

  status = function (data, &value, statement);
  if (status != TRUE)
    return status;

  array = g_ptr_array_new ();

  do {
    g_ptr_array_add (array, value);

    if (separator != TOKEN_NONE && !check_token (data, separator))
      break;

    status = function (data, &value, &statement_one);
    if (status != TRUE) {
      if (status == STATUS_FAIL || separator != TOKEN_NONE)
	return FAIL_CHILD (status);
    } else {
      *statement = vivi_parser_join_statements (*statement, statement_one);
    }
  } while (status == TRUE);
  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);

  return TRUE;
}

static gboolean
parse_value_list (ParseData *data, ParseValueFunction function,
    ViviCodeValue ***list, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;
  gboolean status;

  g_assert (data != NULL);
  g_assert (function != NULL);
  g_assert (list != NULL);

  *list = NULL;

  status = function (data, &value);
  if (status != TRUE)
    return status;

  array = g_ptr_array_new ();

  do {
    g_ptr_array_add (array, value);

    if (separator != TRUE && !check_token (data, separator))
      break;

    status = function (data, &value);
    if (status == STATUS_FAIL)
      return STATUS_FAIL;
  } while (status == TRUE);
  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);

  return TRUE;
}

// public

ViviCodeStatement *
vivi_parse_file (FILE *file, const char *input_name)
{
  ParseData data;
  ViviCodeStatement *statement;
  gboolean status;

  g_return_val_if_fail (file != NULL, NULL);

  data.scanner = vivi_parser_scanner_new (file);
  data.unexpected_line_terminator = FALSE;
  data.expected[0] = TOKEN_NONE;
  data.expected[1] = TOKEN_NONE;
  data.custom_error = NULL;
  data.levels = NULL;
  data.level = NULL;

  status = parse_program (&data, &statement);
  g_assert ((status == TRUE && VIVI_IS_CODE_STATEMENT (statement)) ||
	(status != TRUE && statement == NULL));
  g_assert (status >= 0);

  if (status != TRUE) {
    g_printerr ("%s:%i:%i: error: ", input_name,
	vivi_parser_scanner_cur_line (data.scanner),
	vivi_parser_scanner_cur_column (data.scanner));

    if (data.custom_error != NULL) {
      g_printerr ("%s\n", data.custom_error);
      g_free (data.custom_error);
    } else {
      vivi_parser_scanner_get_next_token (data.scanner);

      g_printerr ("Expected %s ", vivi_parser_token_name (data.expected[0]));
      if (data.expected[1] != TOKEN_NONE)
	g_printerr ("or %s ", vivi_parser_token_name (data.expected[1]));

      if (data.unexpected_line_terminator) {
	g_printerr ("before %s token\n",
	    vivi_parser_token_name (TOKEN_LINE_TERMINATOR));
      } else {
	g_printerr ("before %s token\n",
	    vivi_parser_token_name (data.scanner->token));
      }
    }
  }

  g_object_unref (data.scanner);

  return statement;
}
