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
#include <math.h>

#include "vivi_parser.h"

#include "vivi_parser_scanner.h"

#include "vivi_code_and.h"
#include "vivi_code_assignment.h"
#include "vivi_code_binary_default.h"
#include "vivi_code_block.h"
#include "vivi_code_boolean.h"
#include "vivi_code_break.h"
#include "vivi_code_builtin_statement_default.h"
#include "vivi_code_builtin_value_call_default.h"
#include "vivi_code_builtin_value_statement_default.h"
#include "vivi_code_concat.h"
#include "vivi_code_constant.h"
#include "vivi_code_continue.h"
#include "vivi_code_function.h"
#include "vivi_code_function_call.h"
#include "vivi_code_get.h"
#include "vivi_code_get_timer.h"
#include "vivi_code_get_url.h"
#include "vivi_code_goto.h"
#include "vivi_code_if.h"
#include "vivi_code_init_array.h"
#include "vivi_code_init_object.h"
#include "vivi_code_loop.h"
#include "vivi_code_null.h"
#include "vivi_code_number.h"
#include "vivi_code_or.h"
#include "vivi_code_return.h"
#include "vivi_code_string.h"
#include "vivi_code_substring.h"
#include "vivi_code_throw.h"
#include "vivi_code_unary.h"
#include "vivi_code_undefined.h"
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
  ERROR_TOKEN_FUNCTION_EXPRESSION,
  ERROR_TOKEN_ASSIGNMENT_EXPRESSION,
  ERROR_TOKEN_EXPRESSION,
  ERROR_TOKEN_ITERATION_STATEMENT,
  ERROR_TOKEN_EXPRESSION_STATEMENT,
  ERROR_TOKEN_STATEMENT,
  ERROR_TOKEN_FUNCTION_DECLARATION
};

static const struct {
  guint				token;
  const char *			name;
} error_names[] = {
  { ERROR_TOKEN_LITERAL, "LITERAL" },
  { ERROR_TOKEN_IDENTIFIER, "IDENTIFIER" },
  { ERROR_TOKEN_PROPERTY_NAME, "PROPERTY NAME" },
  { ERROR_TOKEN_PRIMARY_EXPRESSION, "PRIMARY EXPRESSION" },
  { ERROR_TOKEN_FUNCTION_EXPRESSION, "FUNCTION EXPRESSION" },
  { ERROR_TOKEN_ASSIGNMENT_EXPRESSION, "ASSIGNMENT EXPRESSION" },
  { ERROR_TOKEN_EXPRESSION, "EXPRESSION" },
  { ERROR_TOKEN_ITERATION_STATEMENT, "ITERATION STATEMENT" },
  { ERROR_TOKEN_EXPRESSION_STATEMENT, "EXPRESSION STATEMENT" },
  { ERROR_TOKEN_STATEMENT, "STATEMENT" },
  { ERROR_TOKEN_FUNCTION_DECLARATION, "FUNCTION DECLARATION" },
  { TOKEN_NONE, NULL }
};

typedef struct {
  GSList			*labels;
  GSList			*gotos;
} ParseLevel;

typedef struct {
  ViviParserScanner *		scanner;

  GSList *			positions;

  GSList *			levels; // ParseLevel, earlier levels
  ParseLevel *			level;  // current level

  guint				error_count;
} ParseData;

typedef gboolean (*PeekFunction) (ParseData *data);
typedef ViviCodeValue * (*ParseValueFunction) (ParseData *data);
typedef ViviCodeValue * (*ParseValueStatementFunction) (ParseData *data, ViviCodeStatement **statement);
typedef ViviCodeStatement * (*ParseStatementFunction) (ParseData *data);

static void
parse_statement_list (ParseData *data, PeekFunction peek,
    ParseStatementFunction parse, ViviCodeStatement **block, guint separator);

static void
parse_value_statement_list (ParseData *data, PeekFunction peek,
    ParseValueStatementFunction parse, ViviCodeValue ***list,
    ViviCodeStatement **statement, guint separator);

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
    for (i = 0; error_names[i].token != TOKEN_NONE; i++) {
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

  g_printerr ("%i: error: %s\n", data->scanner->next_line_number, message);

  g_free (message);

  data->error_count++;
}

static void
vivi_parser_error_unexpected_or (ParseData *data, ...)
{
  va_list args;
  guint token;
  GString *message;

  g_return_if_fail (data != NULL);

  message = g_string_new ("Expected ");

  va_start (args, data);

  token = va_arg (args, guint);
  g_assert (token != TOKEN_NONE);
  g_string_append (message, vivi_parser_token_name (token));

  while ((token = va_arg (args, guint)) != TOKEN_NONE) {
    g_string_append (message, ", ");
    g_string_append (message, vivi_parser_token_name (token));
  }

  va_end (args);

  g_string_append (message, " before ");

  g_string_append (message,
      vivi_parser_token_name (data->scanner->next_token));

  vivi_parser_error (data, "%s", g_string_free (message, FALSE));
}

static void
vivi_parser_error_unexpected (ParseData *data, guint expected)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (expected != TOKEN_NONE);

  vivi_parser_error_unexpected_or (data, expected, TOKEN_NONE);
}

static void
vivi_parser_error_unexpected_line_terminator (ParseData *data, guint expected)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (expected != TOKEN_NONE);

  vivi_parser_error (data, "Expected %s before %s",
      vivi_parser_token_name (expected),
      vivi_parser_token_name (TOKEN_LINE_TERMINATOR));
}

G_GNUC_WARN_UNUSED_RESULT static gboolean
peek_line_terminator (ParseData *data)
{
  vivi_parser_scanner_peek_next_token (data->scanner);
  return data->scanner->next_line_terminator;
}

G_GNUC_WARN_UNUSED_RESULT static gboolean
peek_token (ParseData *data, ViviParserScannerToken token)
{
  vivi_parser_scanner_peek_next_token (data->scanner);
  return (data->scanner->next_token == token);
}

static void
parse_token (ParseData *data, ViviParserScannerToken token)
{
  vivi_parser_scanner_peek_next_token (data->scanner);
  if (data->scanner->next_token != token) {
    vivi_parser_error_unexpected (data, token);
  } else {
    vivi_parser_scanner_get_next_token (data->scanner);
  }
}

G_GNUC_WARN_UNUSED_RESULT static gboolean
try_parse_token (ParseData *data, ViviParserScannerToken token)
{
  if (!peek_token (data, token))
    return FALSE;

  parse_token (data, token);
  return TRUE;
}

static void
parse_automatic_semicolon (ParseData *data)
{
  if (try_parse_token (data, TOKEN_SEMICOLON))
    return;
  if (peek_line_terminator (data))
    return;

  vivi_parser_scanner_peek_next_token (data->scanner);
  if (data->scanner->next_token == TOKEN_BRACE_LEFT ||
      data->scanner->next_token == TOKEN_EOF)
    return;

  vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);
}

G_GNUC_WARN_UNUSED_RESULT static gboolean
try_parse_restricted_semicolon (ParseData *data)
{
  if (try_parse_token (data, TOKEN_SEMICOLON))
    return TRUE;
  if (peek_line_terminator (data))
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

static gsize
vivi_parser_get_position (ParseData *data)
{
  return data->scanner->position;
}

static void
vivi_parser_start_code_token (ParseData *data)
{
  gsize *position;

  g_return_if_fail (data != NULL);

  position = g_new (gsize, 1);
  *position = vivi_parser_get_position (data);

  data->positions = g_slist_prepend (data->positions, position);
}

static void
vivi_parser_end_code_token (ParseData *data, ViviCodeToken *token)
{
  gsize start;

  g_return_if_fail (data != NULL);
  g_return_if_fail (token == NULL || VIVI_IS_CODE_TOKEN (token));
  g_return_if_fail (data->positions != NULL);

  start = *(gsize *)data->positions->data;
  g_free (data->positions->data);
  data->positions = g_slist_delete_link (data->positions, data->positions);

  // TODO
}

static void
vivi_parser_duplicate_code_token (ParseData *data)
{
  gsize *position;

  g_return_if_fail (data != NULL);
  g_return_if_fail (data->positions != NULL);

  position = g_new (gsize, 1);
  *position = *(gsize *)data->positions->data;

  data->positions = g_slist_prepend (data->positions, position);
}

static void
vivi_parser_error_handler (const char *text, gpointer user_data)
{
  ParseData *data = user_data;

  vivi_parser_error (data, text);
}

// values

/* ActionScript specific */
static gboolean
peek_undefined (ParseData *data)
{
  return peek_token (data, TOKEN_UNDEFINED);
}

static ViviCodeValue *
parse_undefined (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  parse_token (data, TOKEN_UNDEFINED);

  value = vivi_code_undefined_new ();

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static gboolean
peek_null (ParseData *data)
{
  return peek_token (data, TOKEN_NULL);
}

static ViviCodeValue *
parse_null (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  parse_token (data, TOKEN_NULL);

  value = vivi_code_null_new ();

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

G_GNUC_UNUSED static gboolean
peek_boolean_value (ParseData *data)
{
  if (data->scanner->next_value.type == VALUE_TYPE_BOOLEAN) {
    return data->scanner->next_value.v_boolean;
  } else {
    return FALSE;
  }
}

static gboolean
parse_boolean_value (ParseData *data)
{
  parse_token (data, TOKEN_BOOLEAN);

  if (data->scanner->value.type == VALUE_TYPE_BOOLEAN) {
    return data->scanner->value.v_boolean;
  } else {
    return FALSE;
  }
}

static gboolean
peek_boolean (ParseData *data)
{
  return peek_token (data, TOKEN_BOOLEAN);
}

static ViviCodeValue *
parse_boolean (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_boolean_new (parse_boolean_value (data));

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

G_GNUC_UNUSED static double
peek_numeric_value (ParseData *data)
{
  if (data->scanner->next_value.type == VALUE_TYPE_NUMBER) {
    return data->scanner->next_value.v_number;
  } else {
    return NAN;
  }
}

static double
parse_numeric_value (ParseData *data)
{
  parse_token (data, TOKEN_NUMBER);

  if (data->scanner->value.type == VALUE_TYPE_NUMBER) {
    return data->scanner->value.v_number;
  } else {
    return NAN;
  }
}

static gboolean
peek_numeric (ParseData *data)
{
  return peek_token (data, TOKEN_NUMBER);
}

static ViviCodeValue *
parse_numeric (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_number_new (parse_numeric_value (data));

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

G_GNUC_UNUSED static const char *
peek_string_value (ParseData *data)
{
  if (data->scanner->next_value.type == VALUE_TYPE_STRING) {
    return data->scanner->next_value.v_string;
  } else {
    return "undefined";
  }
}

static const char *
parse_string_value (ParseData *data)
{
  parse_token (data, TOKEN_STRING);

  if (data->scanner->value.type == VALUE_TYPE_STRING) {
    return data->scanner->value.v_string;
  } else {
    return "undefined";
  }
}

static gboolean
peek_string (ParseData *data)
{
  return peek_token (data, TOKEN_STRING);
}

static ViviCodeValue *
parse_string (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_string_new (parse_string_value (data));

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static const struct {
  PeekFunction peek;
  ParseValueFunction parse;
} literal_functions[] = {
  { peek_undefined, parse_undefined },
  { peek_null, parse_null },
  { peek_boolean, parse_boolean },
  { peek_numeric, parse_numeric },
  { peek_string, parse_string },
  { NULL, NULL }
};

static gboolean
peek_literal (ParseData *data)
{
  guint i;

  for (i = 0; literal_functions[i].peek != NULL; i++) {
    if (literal_functions[i].peek (data))
      return TRUE;
  }

  return FALSE;
}

static ViviCodeValue *
parse_literal (ParseData *data)
{
  ViviCodeValue *value;
  guint i;

  for (i = 0; literal_functions[i].peek != NULL; i++) {
    if (literal_functions[i].peek (data)) {
      return literal_functions[i].parse (data);
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_LITERAL);
  vivi_parser_start_code_token (data);
  value = vivi_code_undefined_new ();
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static const char *
peek_identifier_value (ParseData *data)
{
  if (data->scanner->next_value.type == VALUE_TYPE_IDENTIFIER) {
    return data->scanner->next_value.v_identifier;
  } else {
    return "undefined";
  }
}

static const char *
parse_identifier_value (ParseData *data)
{
  parse_token (data, TOKEN_IDENTIFIER);

  if (data->scanner->value.type == VALUE_TYPE_IDENTIFIER) {
    return data->scanner->value.v_identifier;
  } else {
    return "undefined";
  }
}

static gboolean
peek_identifier (ParseData *data)
{
  return peek_token (data, TOKEN_IDENTIFIER);
}

static ViviCodeValue *
parse_identifier (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_get_new_name (parse_identifier_value (data));

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static const struct {
  PeekFunction peek;
  ParseValueFunction parse;
} property_name_functions[] = {
  { peek_identifier, parse_identifier },
  { peek_string, parse_string },
  { peek_numeric, parse_numeric },
  { NULL, NULL }
};

G_GNUC_UNUSED static gboolean
peek_property_name (ParseData *data)
{
  guint i;

  for (i = 0; property_name_functions[i].peek != NULL; i++) {
    if (property_name_functions[i].peek (data))
      return TRUE;
  }

  return FALSE;
}

static ViviCodeValue *
parse_property_name (ParseData *data)
{
  ViviCodeValue *value;
  guint i;

  for (i = 0; property_name_functions[i].peek != NULL; i++) {
    if (property_name_functions[i].peek (data)) {
      return property_name_functions[i].parse (data);
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_PROPERTY_NAME);
  vivi_parser_start_code_token (data);
  value = vivi_code_string_new ("undefined");
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static gboolean
peek_assignment_expression (ParseData *data);

static ViviCodeValue *
parse_assignment_expression (ParseData *data, ViviCodeStatement **statement);

static gboolean
peek_array_literal (ParseData *data)
{
  return peek_token (data, TOKEN_BRACKET_LEFT);
}

static ViviCodeValue *
parse_array_literal (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value, *member;
  ViviCodeStatement *statement_new;

  vivi_parser_start_code_token (data);

  value = vivi_code_init_array_new ();
  *statement = NULL;

  parse_token (data, TOKEN_BRACKET_LEFT);

  while (TRUE) {
    if (try_parse_token (data, TOKEN_BRACKET_RIGHT)) {
      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (value),
	 vivi_code_undefined_new ());
      break;
    } else if (try_parse_token (data, TOKEN_COMMA)) {
      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (value),
	 vivi_code_undefined_new ());
    }
    else if (peek_assignment_expression (data))
    {
      member = parse_assignment_expression (data, &statement_new);

      *statement = vivi_parser_join_statements (*statement, statement_new);

      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (value),
	  member);
      g_object_unref (member);

      if (!try_parse_token (data, TOKEN_COMMA)) {
	if (!try_parse_token (data, TOKEN_BRACKET_RIGHT)) {
	  vivi_parser_error_unexpected_or (data, TOKEN_BRACKET_RIGHT,
	      TOKEN_COMMA, TOKEN_NONE);
	}
	break;
      }
    } else {
      vivi_parser_error_unexpected_or (data, TOKEN_BRACKET_RIGHT, TOKEN_COMMA,
	  ERROR_TOKEN_ASSIGNMENT_EXPRESSION, TOKEN_NONE);
      break;
    }
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static gboolean
peek_object_literal (ParseData *data)
{
  return peek_token (data, TOKEN_BRACE_LEFT);
}

static ViviCodeValue *
parse_object_literal (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_init_object_new ();
  *statement = NULL;

  parse_token (data, TOKEN_BRACE_LEFT);

  if (!peek_token (data, TOKEN_BRACE_RIGHT)) {
    do {
      ViviCodeValue *property, *initializer;
      ViviCodeStatement *statement_new;

      property = parse_property_name (data);
      parse_token (data, TOKEN_COLON);
      initializer = parse_assignment_expression (data, &statement_new);

      *statement = vivi_parser_join_statements (*statement, statement_new);

      vivi_code_init_object_add_variable (VIVI_CODE_INIT_OBJECT (value),
	  property, initializer);
    } while (try_parse_token (data, TOKEN_COMMA));
  }

  parse_token (data, TOKEN_BRACE_RIGHT);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

// misc

static gboolean
peek_variable_declaration (ParseData *data)
{
  return peek_identifier (data);
}

static ViviCodeStatement *
parse_variable_declaration (ParseData *data)
{
  ViviCodeValue *identifier, *value;
  ViviCodeStatement *assignment, *statement_right;

  vivi_parser_start_code_token (data);

  identifier = parse_identifier (data);

  if (try_parse_token (data, TOKEN_ASSIGN)) {
    value = parse_assignment_expression (data, &statement_right);
  } else {
    vivi_parser_start_code_token (data);

    value = vivi_code_undefined_new ();
    statement_right = NULL;

    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
  }

  assignment = vivi_parser_assignment_new (identifier, value);
  vivi_code_assignment_set_local (VIVI_CODE_ASSIGNMENT (assignment), TRUE);
  g_object_unref (identifier);
  g_object_unref (value);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (assignment));

  return vivi_parser_join_statements (statement_right, assignment);
}

// builtin functions

static ViviCodeStatement *
parse_get_url2 (ParseData *data, gboolean require_two, gboolean level,
    gboolean internal, gboolean variables)
{
  ViviCodeValue *url, *target;
  ViviCodeStatement *statement, *expression_statement;
  SwfdecLoaderRequest method;

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  url = parse_assignment_expression (data, &statement);

  if (require_two || peek_token (data, TOKEN_COMMA)) {
    parse_token (data, TOKEN_COMMA);

    // target
    if (!level) {
      target = parse_assignment_expression (data, &expression_statement);
      statement =
	vivi_parser_join_statements (statement, expression_statement);
    } else {
      // FIXME: integer only
      if (peek_numeric (data)) {
	char *level_name =
	  g_strdup_printf ("_level%i", (int)parse_numeric_value (data));
	target = vivi_code_string_new (level_name);
	g_free (level_name);
      } else {
	// TODO
	target = parse_assignment_expression (data, &expression_statement);
	statement =
	  vivi_parser_join_statements (statement, expression_statement);
      }
    }

    // method
    if (peek_token (data, TOKEN_COMMA)) {
      parse_token (data, TOKEN_COMMA);

      if (peek_identifier (data) || peek_string (data)) {
	const char *method_string;

	if (peek_identifier (data)) {
	  method_string = parse_identifier_value (data);
	} else {
	  method_string = parse_string_value (data);
	}
	if (g_ascii_strcasecmp (method_string, "GET") == 0) {
	  method = SWFDEC_LOADER_REQUEST_GET;
	} else if (g_ascii_strcasecmp (method_string, "POST") == 0) {
	  method = SWFDEC_LOADER_REQUEST_POST;
	} else {
	  method = SWFDEC_LOADER_REQUEST_DEFAULT;
	  // FIXME: only for identifiers?
	  vivi_parser_error (data, "Invalid URL method: %s\n", method_string);
	}
      } else {
	vivi_parser_error_unexpected_or (data, TOKEN_IDENTIFIER, TOKEN_STRING,
	    TOKEN_NONE);
	method = SWFDEC_LOADER_REQUEST_DEFAULT;
      }
    } else {
      method = SWFDEC_LOADER_REQUEST_DEFAULT;
    }
  } else {
    target = vivi_code_undefined_new ();
    method = SWFDEC_LOADER_REQUEST_DEFAULT;
  }

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  statement = vivi_code_get_url_new (target, url, method, internal, variables);
  g_object_unref (target);
  g_object_unref (url);

  return statement;
}

static ViviCodeStatement *
parse_get_url (ParseData *data)
{
  return parse_get_url2 (data, FALSE, FALSE, FALSE, FALSE);
}

static ViviCodeStatement *
parse_load_movie (ParseData *data)
{
  return parse_get_url2 (data, TRUE, FALSE, TRUE, FALSE);
}

static ViviCodeStatement *
parse_load_movie_num (ParseData *data)
{
  return parse_get_url2 (data, TRUE, TRUE, TRUE, FALSE);
}

static ViviCodeStatement *
parse_load_variables (ParseData *data)
{
  return parse_get_url2 (data, TRUE, FALSE, FALSE, TRUE);
}

static ViviCodeStatement *
parse_load_variables_num (ParseData *data)
{
  return parse_get_url2 (data, TRUE, TRUE, FALSE, TRUE);
}

static ViviCodeValue *
parse_concat (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value, *first, *second;
  ViviCodeStatement *expression_statement;

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  first = parse_assignment_expression (data, statement);

  parse_token (data, TOKEN_COMMA);

  second = parse_assignment_expression (data, &expression_statement);
  *statement = vivi_parser_join_statements (*statement, expression_statement);

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  value = vivi_code_concat_new (first, second);
  g_object_unref (first);
  g_object_unref (second);

  return value;
}

static ViviCodeValue *
parse_substring (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value, *string, *index_, *count;
  ViviCodeStatement *expression_statement;

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  string = parse_assignment_expression (data, statement);

  parse_token (data, TOKEN_COMMA);

  index_ = parse_assignment_expression (data, &expression_statement);
  *statement = vivi_parser_join_statements (*statement, expression_statement);

  parse_token (data, TOKEN_COMMA);

  count = parse_assignment_expression (data, &expression_statement);
  *statement = vivi_parser_join_statements (*statement, expression_statement);

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  value = vivi_code_substring_new (string, index_, count);
  g_object_unref (string);
  g_object_unref (index_);
  g_object_unref (count);

  return value;
}

static ViviCodeValue *
parse_target_path (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value, *identifier;

  *statement = NULL;

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  identifier = parse_identifier (data);

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  value = vivi_code_target_path_new (identifier);
  g_object_unref (identifier);

  return value;
}

typedef ViviCodeStatement *(*NewStatementVoid) (void);
typedef ViviCodeStatement *(*NewStatementValue) (ViviCodeValue *value);

typedef struct {
  const char *			name;
  NewStatementVoid		constructor_void;
  NewStatementValue		constructor_value;
  ParseStatementFunction	parse_custom;
} BuiltinStatement;

static const BuiltinStatement builtin_statements[] = {
  //{ "callFrame",          NULL, vivi_code_call_frame_new, NULL },
  //{ "duplicateMovieClip", NULL, NULL, parse_duplicate_movie_clip },
  //{ "getURL1",            NULL, NULL, parse_get_url1 },
  { "getURL",             NULL, NULL, parse_get_url },
  //{ "gotoAndPlay",        NULL, vivi_code_goto_and_play_new, NULL },
  //{ "gotoAndStop",        NULL, vivi_code_goto_and_stop_new, NULL },
  { "loadMovie",          NULL, NULL, parse_load_movie },
  { "loadMovieNum",       NULL, NULL, parse_load_movie_num },
  { "loadVariables",      NULL, NULL, parse_load_variables },
  { "loadVariablesNum",   NULL, NULL, parse_load_variables_num },
  { "nextFrame",          vivi_code_next_frame_new, NULL, NULL },
  { "play",               vivi_code_play_new, NULL, NULL },
  { "prevFrame",          vivi_code_previous_frame_new, NULL, NULL },
  { "removeMovieClip",    NULL, vivi_code_remove_sprite_new, NULL },
  //{ "setProperty",        NULL, NULL, parse_set_property },
  { "setTarget",          NULL, vivi_code_set_target2_new, NULL },
  //{ "startDrag",          NULL, NULL, parse_start_drag },
  { "stop",               vivi_code_stop_new, NULL, NULL },
  { "stopDrag",           vivi_code_end_drag_new, NULL, NULL },
  { "stopSounds",         vivi_code_stop_sounds_new, NULL, NULL },
  { "toggleQuality",      vivi_code_toggle_quality_new, NULL, NULL },
  { "trace",              NULL, vivi_code_trace_new, NULL }
};

typedef ViviCodeValue *(*NewCallVoid) (void);
typedef ViviCodeValue *(*NewCallValue) (ViviCodeValue *value);

typedef struct {
  const char *			name;
  NewCallVoid			constructor_void;
  NewCallValue			constructor_value;
  ParseValueStatementFunction	parse_custom;
} BuiltinCall;

static const BuiltinCall builtin_calls[] = {
  { "chr",         NULL, vivi_code_ascii_to_char_new, NULL },
  { "concat",      NULL, NULL, parse_concat },
  { "eval",        NULL, vivi_code_get_variable_new, NULL },
  //{ "getProperty", NULL, NULL, parse_get_property },
  { "getTimer",    vivi_code_get_timer_new, NULL, NULL },
  { "int",         NULL, vivi_code_to_integer_new, NULL },
  { "length",      NULL, vivi_code_string_length_new, NULL },
  { "ord",         NULL, vivi_code_char_to_ascii_new, NULL },
  { "random",      NULL, vivi_code_random_new, NULL },
  { "substring",   NULL, NULL, parse_substring },
  { "targetPath",  NULL, NULL, parse_target_path },
  { "typeOf",      NULL, vivi_code_type_of_new, NULL }
};

static gboolean
peek_builtin_call (ParseData *data)
{
  guint i;
  const char *identifier;

  if (!peek_token (data, TOKEN_IDENTIFIER))
    return FALSE;

  identifier = data->scanner->next_value.v_identifier;

  // TODO: Check that ( follows?

  for (i = 0; i < G_N_ELEMENTS (builtin_calls); i++) {
    if (g_ascii_strcasecmp (identifier, builtin_calls[i].name) == 0)
      return TRUE;
  }

  return FALSE;
}

static ViviCodeValue *
parse_builtin_call (ParseData *data, ViviCodeStatement **statement)
{
  guint i;
  const char *identifier;
  ViviCodeValue *value, *argument;

  identifier = parse_identifier_value (data);

  for (i = 0; i < G_N_ELEMENTS (builtin_calls); i++) {
    if (g_ascii_strcasecmp (identifier, builtin_calls[i].name) == 0)
      break;
  }
  if (i >= G_N_ELEMENTS (builtin_calls)) {
    vivi_parser_error (data, "Unknown builtin call: %s", identifier);
    i = 0;
  }

  if (builtin_calls[i].parse_custom != NULL)
    return builtin_calls[i].parse_custom (data, statement);

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  if (builtin_calls[i].constructor_value != NULL)
    argument = parse_assignment_expression (data, statement);

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  if (builtin_calls[i].constructor_value != NULL) {
    value = builtin_calls[i].constructor_value (argument);
    g_object_unref (argument);
  } else {
    g_assert (builtin_calls[i].constructor_void != NULL);
    value = builtin_calls[i].constructor_void ();
  }

  return value;
}


static gboolean
peek_builtin_statement (ParseData *data)
{
  guint i;
  const char *identifier;

  if (!peek_token (data, TOKEN_IDENTIFIER))
    return FALSE;

  identifier = peek_identifier_value (data);

  // TODO: Check that ( follows?

  for (i = 0; i < G_N_ELEMENTS (builtin_statements); i++) {
    if (g_ascii_strcasecmp (identifier, builtin_statements[i].name) == 0)
      return TRUE;
  }

  return FALSE;
}

static ViviCodeStatement *
parse_builtin_statement (ParseData *data)
{
  guint i;
  const char *identifier;
  ViviCodeValue *argument;
  ViviCodeStatement *statement, *argument_statement;

  identifier = parse_identifier_value (data);

  for (i = 0; i < G_N_ELEMENTS (builtin_statements); i++) {
    if (g_ascii_strcasecmp (identifier, builtin_statements[i].name) == 0)
      break;
  }
  if (i >= G_N_ELEMENTS (builtin_statements)) {
    vivi_parser_error (data, "Unknown builtin statement: %s", identifier);
    i = 0;
  }

  if (builtin_statements[i].parse_custom != NULL) {
    return builtin_statements[i].parse_custom (data);
  }

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  if (builtin_statements[i].constructor_value != NULL)
    argument = parse_assignment_expression (data, &argument_statement);

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  parse_automatic_semicolon (data);

  if (builtin_statements[i].constructor_value != NULL) {
    statement = builtin_statements[i].constructor_value (argument);
    g_object_unref (argument);
    statement = vivi_parser_join_statements (argument_statement, statement);
  } else {
    g_assert (builtin_statements[i].constructor_void != NULL);
    statement = builtin_statements[i].constructor_void ();
  }

  return statement;
}

// expression

static const struct {
  PeekFunction peek;
  ParseValueFunction parse_value;
  ParseValueStatementFunction parse_value_statement;
} primary_expression_functions[] = {
  { peek_object_literal, NULL, parse_object_literal },
  { peek_array_literal, NULL, parse_array_literal },
  { peek_identifier, parse_identifier, NULL },
  { peek_literal, parse_literal, NULL },
  { NULL, NULL, NULL }
};

static gboolean
peek_primary_expression (ParseData *data)
{
  guint i;

  if (peek_token (data, TOKEN_THIS)||
      peek_token (data, TOKEN_PARENTHESIS_LEFT))
    return TRUE;

  for (i = 0; primary_expression_functions[i].peek != NULL; i++) {
    if (primary_expression_functions[i].peek (data))
      return TRUE;
  }

  return FALSE;
}

static ViviCodeValue *
parse_expression (ParseData *data, ViviCodeStatement **statement);

static ViviCodeValue *
parse_primary_expression (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value;
  guint i;

  vivi_parser_start_code_token (data);

  if (try_parse_token (data, TOKEN_THIS)) {
    value = vivi_code_get_new_name ("this");
    *statement = NULL;

    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

    return value;
  }

  if (try_parse_token (data, TOKEN_PARENTHESIS_LEFT)) {
    value = parse_expression (data, statement);
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);

    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

    return value;
  }

  for (i = 0; primary_expression_functions[i].peek != NULL; i++) {
    if (primary_expression_functions[i].peek (data)) {
      if (primary_expression_functions[i].parse_value != NULL) {
	value = primary_expression_functions[i].parse_value (data);
	*statement = NULL;
      } else {
	value = primary_expression_functions[i].parse_value_statement (data,
	    statement);
      }
      vivi_parser_end_code_token (data, NULL);

      return value;
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_PRIMARY_EXPRESSION);
  value = vivi_code_undefined_new ();
  *statement = NULL;
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static gboolean
peek_function_expression (ParseData *data);

static ViviCodeValue *
parse_function_expression (ParseData *data);

static gboolean
peek_member_expression (ParseData *data)
{
  return (peek_primary_expression (data) || peek_function_expression (data));
}

static ViviCodeValue *
parse_member_expression (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value, *member;
  ViviCodeStatement *statement_member;

  vivi_parser_start_code_token (data);

  if (peek_primary_expression (data)) {
    value = parse_primary_expression (data, statement);
  } else if (peek_function_expression (data)) {
    value = parse_function_expression (data);
    *statement = NULL;
  } else {
    vivi_parser_error_unexpected_or (data, ERROR_TOKEN_PRIMARY_EXPRESSION,
	ERROR_TOKEN_FUNCTION_EXPRESSION, TOKEN_NONE);
    value = vivi_code_undefined_new ();
    *statement = NULL;

    vivi_parser_duplicate_code_token (data);
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
  }

  while (TRUE) {
    ViviCodeValue *tmp;

    if (try_parse_token (data, TOKEN_BRACKET_LEFT)) {
      member = parse_expression (data, &statement_member);

      *statement = vivi_parser_join_statements (*statement, statement_member);

      parse_token (data, TOKEN_BRACKET_RIGHT);
    } else if (try_parse_token (data, TOKEN_DOT)) {
      member = parse_identifier (data);
    } else {
      vivi_parser_end_code_token (data, NULL);
      return value;
    }

    tmp = value;
    value = vivi_parser_get_new (tmp, member);
    g_object_unref (tmp);
    g_object_unref (member);

    vivi_parser_duplicate_code_token (data);
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
  }

  g_assert_not_reached ();
}

static gboolean
peek_left_hand_side_expression (ParseData *data)
{
  return (peek_token (data, TOKEN_NEW) || peek_member_expression (data));

}

static ViviCodeValue *
parse_left_hand_side_expression (ParseData *data,
    ViviCodeStatement **statement)
{
  ViviCodeValue *value, *name;
  ViviCodeValue **arguments;
  ViviCodeStatement *argument_statement;
  guint i;

  vivi_parser_start_code_token (data);

  if (try_parse_token (data, TOKEN_NEW)) {
    value = parse_left_hand_side_expression (data, statement);

    if (!VIVI_IS_CODE_FUNCTION_CALL (value)) {
      ViviCodeValue *tmp = value;
      value = vivi_parser_function_call_new (tmp);
      g_object_unref (tmp);
    }

    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (value),
	TRUE);

    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

    return value;
  }

  if (peek_builtin_call (data)) {
    value = parse_builtin_call (data, statement);
  } else {
    value = parse_member_expression (data, statement);
  }

  while (try_parse_token (data, TOKEN_PARENTHESIS_LEFT)) {
    if (!try_parse_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      parse_value_statement_list (data, peek_assignment_expression,
	  parse_assignment_expression, &arguments, &argument_statement,
	  TOKEN_COMMA);

      *statement =
	vivi_parser_join_statements (*statement, argument_statement);

      parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    } else {
      arguments = NULL;
    }

    name = value;
    value = vivi_parser_function_call_new (name);
    g_object_unref (name);

    if (arguments != NULL) {
      for (i = 0; arguments[i] != NULL; i++) {
	vivi_code_function_call_add_argument (VIVI_CODE_FUNCTION_CALL (value),
	    arguments[i]);
      }
      free_value_list (arguments);
    }

    vivi_parser_duplicate_code_token (data);
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
  }

  vivi_parser_end_code_token (data, NULL);

  return value;
}

static gboolean
peek_postfix_expression (ParseData *data)
{
  return peek_left_hand_side_expression (data);
}

static ViviCodeValue *
parse_postfix_expression (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeStatement *assignment;
  ViviCodeValue *value, *operation, *one, *temporary;
  gboolean add;

  vivi_parser_start_code_token (data);

  value = parse_left_hand_side_expression (data, statement);

  if (peek_line_terminator (data)) {
    vivi_parser_end_code_token (data, NULL);
    return value;
  }

  if (try_parse_token (data, TOKEN_INCREASE)) {
    add = TRUE;
  } else if (try_parse_token (data, TOKEN_DESCREASE)) {
    add = FALSE;
  } else {
    vivi_parser_end_code_token (data, NULL);
    return value;
  }

  // FIXME: Correct?
  if (!vivi_parser_value_is_left_hand_side (value)) {
    vivi_parser_error (data,
	"Invalid left-hand side expression for INCREASE/DECREASE");
  }

  one = vivi_code_number_new (1);
  operation = (add ? vivi_code_add_new : vivi_code_subtract_new) (value, one);
  g_object_unref (one);

  vivi_parser_duplicate_code_token (data);
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (operation));

  assignment = vivi_parser_assignment_new (value, operation);
  g_object_unref (operation);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (assignment));

  temporary = vivi_compiler_get_temporary_new ();
  *statement = vivi_parser_join_statements (*statement,
     vivi_parser_join_statements (
       vivi_parser_assignment_new (temporary, value), assignment));

  g_object_unref (value);
  value = temporary;

  return value;
}

static gboolean
peek_unary_expression (ParseData *data)
{
  vivi_parser_scanner_peek_next_token (data->scanner);
  switch ((guint)data->scanner->next_token) {
    /*case TOKEN_DELETE:
    case TOKEN_VOID:
    case TOKEN_TYPEOF:*/
    case TOKEN_INCREASE:
    case TOKEN_DESCREASE:
    /*case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BITWISE_NOT:*/
    case TOKEN_LOGICAL_NOT:
      return TRUE;
    default:
      return peek_postfix_expression (data);
  }

  g_assert_not_reached ();
}

static ViviCodeValue *
parse_unary_expression (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value, *tmp, *one;

  vivi_parser_scanner_peek_next_token (data->scanner);
  switch ((guint)data->scanner->next_token) {
    /*case TOKEN_DELETE:
    case TOKEN_VOID:
    case TOKEN_TYPEOF:*/
    case TOKEN_INCREASE:
    case TOKEN_DESCREASE:
      vivi_parser_start_code_token (data);

      vivi_parser_scanner_get_next_token (data->scanner);

      value = parse_unary_expression (data, statement);

      // FIXME: Correct?
      if (!vivi_parser_value_is_left_hand_side (value)) {
	vivi_parser_error (data,
	    "Invalid left-hand side expression for INCREASE/DECREASE");
      }

      one = vivi_code_number_new (1);
      tmp = (data->scanner->next_token == TOKEN_INCREASE ?
	  vivi_code_add_new : vivi_code_subtract_new) (value, one);
      g_object_unref (one);

      vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (tmp));

      *statement = vivi_parser_join_statements (*statement,
	  vivi_parser_assignment_new (value, tmp));
      g_object_unref (tmp);
      break;
    /*case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BITWISE_NOT:*/
    case TOKEN_LOGICAL_NOT:
      vivi_parser_start_code_token (data);

      vivi_parser_scanner_get_next_token (data->scanner);

      value = parse_unary_expression (data, statement);

      tmp = VIVI_CODE_VALUE (value);
      value = vivi_code_unary_new (tmp, '!');
      g_object_unref (tmp);

      vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (tmp));
      break;
    default:
      value = parse_postfix_expression (data, statement);
      break;
  }

  return value;
}

typedef enum {
  PASS_ALWAYS,
  PASS_LOGICAL_OR,
  PASS_LOGICAL_AND
} ParseOperatorPass;

static ViviCodeValue *
parse_operator_expression (ParseData *data, ViviCodeStatement **statement,
    const ViviParserScannerToken *tokens, ParseOperatorPass pass,
    ParseValueStatementFunction next_parse_function)
{
  ViviCodeValue *value, *left, *right;
  ViviCodeStatement *statement_right;
  guint i, j;
  static const struct {
    ViviParserScannerToken	token;
    ViviCodeValue *		(* func) (ViviCodeValue *, ViviCodeValue *);
  } table[] = {
//    { TOKEN_, vivi_code_add_new },
    { TOKEN_MINUS, vivi_code_subtract_new },
    { TOKEN_MULTIPLY, vivi_code_multiply_new },
    { TOKEN_DIVIDE, vivi_code_divide_new },
    { TOKEN_REMAINDER, vivi_code_modulo_new },
//    { TOKEN_, vivi_code_equals_new },
//    { TOKEN_, vivi_code_less_new },
//    { TOKEN_, vivi_code_logical_and_new },
//    { TOKEN_, vivi_code_logical_or_new },
//    { TOKEN_, vivi_code_string_equals_new },
//    { TOKEN_, vivi_code_string_less_new },
    { TOKEN_PLUS, vivi_code_add2_new },
    { TOKEN_LESS_THAN, vivi_code_less2_new },
    { TOKEN_EQUAL, vivi_code_equals2_new },
//    { TOKEN_BITWISE_AND, vivi_code_bitwise_and_new },
//    { TOKEN_BITWISE_OR, vivi_code_bitwise_or_new },
//    { TOKEN_BITWISE_XOR, vivi_code_bitwise_xor_new },
//    { TOKEN_SHIFT_LEFT, vivi_code_left_shift_new },
//    { TOKEN_SHIFT_RIGHT, vivi_code_right_shift_new },
//    { TOKEN_SHIFT_RIGHT_UNSIGNED, vivi_code_unsigned_right_shift_new },
    { TOKEN_STRICT_EQUAL, vivi_code_strict_equals_new },
    { TOKEN_GREATER_THAN, vivi_code_greater_new },
//    { TOKEN_, vivi_code_string_greater_new },
    { TOKEN_LOGICAL_AND, vivi_code_and_new },
    { TOKEN_LOGICAL_OR, vivi_code_or_new }
  };

  vivi_parser_start_code_token (data);

  value = next_parse_function (data, statement);

again:
  for (i = 0; tokens[i] != TOKEN_NONE; i++) {
    if (try_parse_token (data, tokens[i])) {
      right = next_parse_function (data, &statement_right);

      if (statement_right != NULL) {
	ViviCodeStatement *tmp;

	switch (pass) {
	  case PASS_LOGICAL_OR:
	    tmp = vivi_code_if_new (vivi_code_unary_new (value, '!'));
	    vivi_code_if_set_if (VIVI_CODE_IF (tmp), statement_right);
	    g_object_unref (statement_right);
	    statement_right = tmp;
	    break;
	  case PASS_LOGICAL_AND:
	    tmp = vivi_code_if_new (value);
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

      left = VIVI_CODE_VALUE (value);
      for (j = 0; j < G_N_ELEMENTS (table); j++) {
	if (tokens[i] != table[j].token)
	  continue;

	value = table[j].func (left, VIVI_CODE_VALUE (right));
	g_object_unref (left);
	g_object_unref (right);

	vivi_parser_duplicate_code_token (data);
	vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

	goto again;
      }
      g_assert_not_reached ();
    }
  }

  vivi_parser_end_code_token (data, NULL);

  return value;
}

static gboolean
peek_multiplicative_expression (ParseData *data)
{
  return peek_unary_expression (data);
}

static ViviCodeValue *
parse_multiplicative_expression (ParseData *data,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_MULTIPLY,
    TOKEN_DIVIDE, TOKEN_REMAINDER, TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_ALWAYS,
      parse_unary_expression);
}

static gboolean
peek_additive_expression (ParseData *data)
{
  return peek_multiplicative_expression (data);
}

static ViviCodeValue *
parse_additive_expression (ParseData *data, ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_ALWAYS,
      parse_multiplicative_expression);
}

static gboolean
peek_shift_expression (ParseData *data)
{
  return peek_additive_expression (data);
}

static ViviCodeValue *
parse_shift_expression (ParseData *data, ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT, TOKEN_SHIFT_RIGHT_UNSIGNED, TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_ALWAYS,
      parse_additive_expression);
}

static gboolean
peek_relational_expression (ParseData *data)
{
  return peek_shift_expression (data);
}

static ViviCodeValue *
parse_relational_expression (ParseData *data, ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN, /*TOKEN_LESS_THAN_OR_EQUAL,
    TOKEN_EQUAL_OR_GREATER_THAN, TOKEN_INSTANCEOF, TOKEN_IN,*/ TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_ALWAYS,
      parse_shift_expression);
}

static gboolean
peek_equality_expression (ParseData *data)
{
  return peek_relational_expression (data);
}

static ViviCodeValue *
parse_equality_expression (ParseData *data, ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_EQUAL,
    /*TOKEN_NOT_EQUAL,*/ TOKEN_STRICT_EQUAL, /*TOKEN_NOT_STRICT_EQUAL,*/
    TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_ALWAYS,
      parse_relational_expression);
}

static gboolean
peek_bitwise_and_expression (ParseData *data)
{
  return peek_equality_expression (data);
}

static ViviCodeValue *
parse_bitwise_and_expression (ParseData *data, ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_AND,
    TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_ALWAYS,
      parse_equality_expression);
}

static gboolean
peek_bitwise_xor_expression (ParseData *data)
{
  return peek_bitwise_and_expression (data);
}

static ViviCodeValue *
parse_bitwise_xor_expression (ParseData *data, ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_XOR,
    TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_ALWAYS,
      parse_bitwise_and_expression);
}

static gboolean
peek_bitwise_or_expression (ParseData *data)
{
  return peek_bitwise_xor_expression (data);
}

static ViviCodeValue *
parse_bitwise_or_expression (ParseData *data, ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_OR,
    TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_ALWAYS,
      parse_bitwise_xor_expression);
}

static gboolean
peek_logical_and_expression (ParseData *data)
{
  return peek_bitwise_or_expression (data);
}

static ViviCodeValue *
parse_logical_and_expression (ParseData *data, ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LOGICAL_AND,
    TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_LOGICAL_AND,
      parse_bitwise_or_expression);
}

static gboolean
peek_logical_or_expression (ParseData *data)
{
  return peek_logical_and_expression (data);
}

static ViviCodeValue *
parse_logical_or_expression (ParseData *data, ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LOGICAL_OR,
    TOKEN_NONE };

  return parse_operator_expression (data, statement, tokens, PASS_LOGICAL_OR,
      parse_logical_and_expression);
}

static gboolean
peek_conditional_expression (ParseData *data)
{
  return peek_logical_or_expression (data);
}

static ViviCodeValue *
parse_conditional_expression (ParseData *data, ViviCodeStatement **statement)
{
  //ViviCodeStatement *if_statement, *else_statement;

  return parse_logical_or_expression (data, statement);

#if 0
  if (!parse_token (data, TOKEN_QUESTION_MARK)) {
    *statement = vivi_code_value_statement_new (VIVI_CODE_VALUE (value));
    g_object_unref (value);
    return TRUE;
  }

  status = parse_assignment_expression (data, &if_statement);
  if (status != TRUE) {
    g_object_unref (value);
    return FAIL_CHILD (status);
  }

  if (!parse_token (data, TOKEN_COLON)) {
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
}

static gboolean
peek_assignment_expression (ParseData *data)
{
  return peek_conditional_expression (data);
}

static ViviCodeValue *
parse_assignment_expression (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value, *right;
  ViviCodeStatement *assignment, *statement_right;
  ViviCodeValue * (* func) (ViviCodeValue *, ViviCodeValue *);

  vivi_parser_start_code_token (data);

  value = parse_conditional_expression (data, statement);

  // FIXME: Correct?
  if (!vivi_parser_value_is_left_hand_side (value)) {
    vivi_parser_end_code_token (data, NULL);
    return value;
  }

  vivi_parser_scanner_peek_next_token (data->scanner);
  switch ((guint)data->scanner->next_token) {
    case TOKEN_ASSIGN_MULTIPLY:
      func = vivi_code_multiply_new;
      break;
    case TOKEN_ASSIGN_DIVIDE:
      func = vivi_code_divide_new;
      break;
    case TOKEN_ASSIGN_REMAINDER:
      func = vivi_code_modulo_new;
      break;
    case TOKEN_ASSIGN_ADD:
      func = vivi_code_add2_new;
      break;
    case TOKEN_ASSIGN_MINUS:
      func = vivi_code_subtract_new;
      break;
    //case TOKEN_ASSIGN_SHIFT_LEFT:
    //  func = vivi_code_left_shift_new;
    //  break;
    //case TOKEN_ASSIGN_SHIFT_RIGHT:
    //  func = vivi_code_right_shift_new;
    //  break;
    //case TOKEN_ASSIGN_SHIFT_RIGHT_ZERO:
    //  func = vivi_code_unsigned_right_shift_new;
    //  break;
    //case TOKEN_ASSIGN_BITWISE_AND:
    //  func = vivi_code_bitwise_and_new;
    //  break;
    //case TOKEN_ASSIGN_BITWISE_OR:
    //  func = vivi_code_bitwise_or_new;
    //  break;
    //case TOKEN_ASSIGN_BITWISE_XOR:
    //  func = vivi_code_bitwise_xor_new;
    //  break;
    case TOKEN_ASSIGN:
      func = NULL;
      break;
    default:
      return value;
  }

  vivi_parser_scanner_get_next_token (data->scanner);

  right = parse_assignment_expression (data, &statement_right);

  if (func != NULL) {
    assignment = vivi_parser_assignment_new (value, func (value, right));
  } else {
    assignment = vivi_parser_assignment_new (value, right);
  }
  g_object_unref (right);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (assignment));

  *statement = vivi_parser_join_statements (*statement,
      vivi_parser_join_statements (statement_right, assignment));

  return value;
}

static gboolean
peek_expression (ParseData *data)
{
  return peek_assignment_expression (data);
}

static ViviCodeValue *
parse_expression (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value;
  ViviCodeStatement *statement_one;

  value = NULL;
  *statement = NULL;

  do {
    if (value != NULL)
      g_object_unref (value);
    value = parse_assignment_expression (data, &statement_one);
    *statement = vivi_parser_join_statements (*statement, statement_one);
  } while (try_parse_token (data, TOKEN_COMMA));

  return value;
}

// statement

static ViviCodeStatement *
parse_continue_or_break_statement (ParseData *data,
    ViviParserScannerToken token)
{
  ViviCodeStatement *statement;

  vivi_parser_start_code_token (data);

  parse_token (data, token);

  if (!try_parse_restricted_semicolon (data)) {
    statement = vivi_compiler_goto_name_new (parse_identifier_value (data));

    vivi_parser_add_goto (data, VIVI_COMPILER_GOTO_NAME (statement));

    parse_automatic_semicolon (data);
  } else {
    // FIXME
    statement = vivi_code_break_new ();
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (statement));

  return statement;
}

static gboolean
peek_continue_statement (ParseData *data)
{
  return peek_token (data, TOKEN_CONTINUE);
}

static ViviCodeStatement *
parse_continue_statement (ParseData *data)
{
  return parse_continue_or_break_statement (data, TOKEN_CONTINUE);
}

static gboolean
peek_break_statement (ParseData *data)
{
  return peek_token (data, TOKEN_BREAK);
}

static ViviCodeStatement *
parse_break_statement (ParseData *data)
{
  return parse_continue_or_break_statement (data, TOKEN_BREAK);
}

static gboolean
peek_throw_statement (ParseData *data)
{
  return peek_token (data, TOKEN_THROW);
}

static ViviCodeStatement *
parse_throw_statement (ParseData *data)
{
  ViviCodeValue *value;
  ViviCodeStatement *statement, *expression_statement;

  parse_token (data, TOKEN_THROW);

  if (peek_line_terminator (data)) {
    vivi_parser_error_unexpected_line_terminator (data,
	ERROR_TOKEN_EXPRESSION);
  }

  value = parse_expression (data, &expression_statement);

  statement = vivi_code_throw_new (value);
  g_object_unref (value);

  statement = vivi_parser_join_statements (expression_statement, statement);

  parse_automatic_semicolon (data);

  return statement;
}

static gboolean
peek_return_statement (ParseData *data)
{
  return peek_token (data, TOKEN_RETURN);
}

static ViviCodeStatement *
parse_return_statement (ParseData *data)
{
  ViviCodeStatement *statement;

  statement = vivi_code_return_new ();

  parse_token (data, TOKEN_RETURN);

  if (!try_parse_restricted_semicolon (data)) {
    ViviCodeValue *value;
    ViviCodeStatement *expression_statement;

    value = parse_expression (data, &expression_statement);

    vivi_code_return_set_value (VIVI_CODE_RETURN (statement), value);
    g_object_unref (value);

    statement = vivi_parser_join_statements (expression_statement, statement);

    parse_automatic_semicolon (data);
  }

  return statement;
}

static gboolean
peek_statement (ParseData *data);

static ViviCodeStatement *
parse_statement (ParseData *data);

static gboolean
peek_iteration_statement (ParseData *data)
{
  return (peek_token (data, TOKEN_DO) || peek_token (data, TOKEN_WHILE) ||
      peek_token (data, TOKEN_FOR));
}

static ViviCodeStatement *
parse_iteration_statement (ParseData *data)
{
  ViviCodeValue *condition;
  ViviCodeStatement *statement;
  ViviCodeStatement *pre_statement, *condition_statement, *loop_statement;

  if (try_parse_token (data, TOKEN_DO))
  {
    loop_statement = parse_statement (data);
    parse_token (data, TOKEN_WHILE);
    parse_token (data, TOKEN_PARENTHESIS_LEFT);
    condition = parse_expression (data, &condition_statement);
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    parse_automatic_semicolon (data);

    pre_statement = g_object_ref (loop_statement);
  }
  else if (try_parse_token (data, TOKEN_WHILE))
  {
    parse_token (data, TOKEN_PARENTHESIS_LEFT);
    condition = parse_expression (data, &condition_statement);
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    loop_statement = parse_statement (data);

    pre_statement = NULL;
  }
  else if (try_parse_token (data, TOKEN_FOR))
  {
    ViviCodeValue *pre_value;
    ViviCodeStatement *post_statement;

    parse_token (data, TOKEN_PARENTHESIS_LEFT);

    if (try_parse_token (data, TOKEN_VAR)) {
      // FIXME: no in
      parse_statement_list (data, peek_variable_declaration,
	  parse_variable_declaration, &pre_statement, TOKEN_COMMA);
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
      if (try_parse_token (data, TOKEN_SEMICOLON)) {
	pre_value = NULL;
	pre_statement = NULL;
      } else {
	// FIXME: no in
	pre_value = parse_expression (data, &pre_statement);
      }
    }

    if (try_parse_token (data, TOKEN_SEMICOLON)) {
      if (pre_value != NULL)
	g_object_unref (pre_value);

      if (try_parse_token (data, TOKEN_SEMICOLON)) {
	condition = vivi_code_boolean_new (TRUE);
	condition_statement = NULL;
      } else {
	condition = parse_expression (data, &condition_statement);
	parse_token (data, TOKEN_SEMICOLON);
      }

      if (peek_token (data, TOKEN_PARENTHESIS_RIGHT)) {
	post_statement = NULL;
      } else {
	pre_value = parse_expression (data, &post_statement);
	g_object_unref (pre_value);
      }
    } else if (pre_value != NULL && try_parse_token (data, TOKEN_IN)) {
      post_statement = NULL;

      // FIXME: correct?
      if (!vivi_parser_value_is_left_hand_side (pre_value))
	vivi_parser_error (data, "Invalid left-hand side expression for in");

      g_object_unref (pre_value);
      if (pre_statement != NULL)
	g_object_unref (pre_statement);

      vivi_parser_error (data, "for (... in ...) has not been implemented yet");

      condition = vivi_code_undefined_new ();
      condition_statement = NULL;
      post_statement = NULL;
    } else {
      if (pre_value != NULL) {
	vivi_parser_error_unexpected_or (data, TOKEN_SEMICOLON, TOKEN_IN,
	    TOKEN_NONE);
      } else {
	vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);
      }

      condition = vivi_code_undefined_new ();
      condition_statement = NULL;
      post_statement = NULL;

      if (pre_value != NULL)
	g_object_unref (pre_value);
      if (pre_statement != NULL)
	g_object_unref (pre_statement);
    }

    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    loop_statement = parse_statement (data);

    loop_statement =
      vivi_parser_join_statements (loop_statement, post_statement);
  }
  else
  {
    vivi_parser_error_unexpected (data, ERROR_TOKEN_ITERATION_STATEMENT);

    condition = vivi_code_undefined_new ();
    pre_statement = NULL;
    condition_statement = NULL;
    loop_statement = vivi_compiler_empty_statement_new ();
  }

  if (condition_statement != NULL) {
    pre_statement = vivi_parser_join_statements (pre_statement,
	g_object_ref (condition_statement));
    loop_statement = vivi_parser_join_statements (loop_statement,
	g_object_ref (condition_statement));
    g_object_unref (condition_statement);
  }

  statement = vivi_code_loop_new ();
  vivi_code_loop_set_condition (VIVI_CODE_LOOP (statement), condition);
  g_object_unref (condition);
  vivi_code_loop_set_statement (VIVI_CODE_LOOP (statement), loop_statement);
  g_object_unref (loop_statement);

  statement = vivi_parser_join_statements (pre_statement, statement);

  return statement;
}

static gboolean
peek_if_statement (ParseData *data)
{
  return peek_token (data, TOKEN_IF);
}

static ViviCodeStatement *
parse_if_statement (ParseData *data)
{
  ViviCodeValue *condition;
  ViviCodeStatement *statement, *pre_statement, *if_statement, *else_statement;

  parse_token (data, TOKEN_IF);
  parse_token (data, TOKEN_PARENTHESIS_LEFT);
  condition = parse_expression (data, &pre_statement);
  parse_token (data, TOKEN_PARENTHESIS_RIGHT);
  if_statement = parse_statement (data);

  if (try_parse_token (data, TOKEN_ELSE)) {
    else_statement = parse_statement (data);
  } else {
    else_statement = NULL;
  }

  statement = vivi_code_if_new (condition);
  g_object_unref (condition);

  vivi_code_if_set_if (VIVI_CODE_IF (statement), if_statement);
  g_object_unref (if_statement);

  if (else_statement != NULL) {
    vivi_code_if_set_else (VIVI_CODE_IF (statement), else_statement);
    g_object_unref (else_statement);
  }

  statement = vivi_parser_join_statements (pre_statement, statement);

  return statement;
}

static gboolean
peek_expression_statement (ParseData *data)
{
  if (peek_token (data, TOKEN_BRACE_LEFT) || peek_token (data, TOKEN_FUNCTION))
    return FALSE;

  return peek_expression (data);
}

static ViviCodeStatement *
parse_expression_statement (ParseData *data)
{
  ViviCodeValue *value;
  ViviCodeStatement *statement, *last;

  if (peek_token (data, TOKEN_BRACE_LEFT) || peek_token (data, TOKEN_FUNCTION))
    vivi_parser_error_unexpected (data, ERROR_TOKEN_EXPRESSION_STATEMENT);

  value = parse_expression (data, &statement);

  // check for label
  if (statement == NULL && vivi_parser_value_is_identifier (value) &&
      try_parse_token (data, TOKEN_COLON))
  {
    statement = vivi_code_label_new (vivi_code_constant_get_variable_name (
	  VIVI_CODE_CONSTANT (VIVI_CODE_GET (value)->name)));
    if (!vivi_parser_add_label (data, VIVI_CODE_LABEL (statement)))
      vivi_parser_error (data, "Same label name used twice");
  }

  parse_automatic_semicolon (data);

  // add a value statement, if the last statement is not an assignment with the
  // same value
  if (VIVI_IS_CODE_BLOCK (statement)) {
    ViviCodeBlock *block = VIVI_CODE_BLOCK (statement);

    last = vivi_code_block_get_statement (block,
	vivi_code_block_get_n_statements (block) - 1);
  } else {
    last = statement;
  }

  if (VIVI_IS_CODE_ASSIGNMENT (last) && VIVI_IS_CODE_GET (value)) {
    ViviCodeAssignment *assignment = VIVI_CODE_ASSIGNMENT (last);

    if (assignment->from == NULL && assignment->name == VIVI_CODE_GET (value)->name) {
      g_object_unref (value);
      return statement;
    }
  }

  statement = vivi_parser_join_statements (statement,
      vivi_code_value_statement_new (value));
  g_object_unref (value);

  return statement;
}

static gboolean
peek_empty_statement (ParseData *data)
{
  return peek_token (data, TOKEN_SEMICOLON);
}

static ViviCodeStatement *
parse_empty_statement (ParseData *data)
{
  parse_token (data, TOKEN_SEMICOLON);

  return vivi_compiler_empty_statement_new ();
}

static gboolean
peek_block (ParseData *data)
{
  return peek_token (data, TOKEN_BRACE_LEFT);
}

static ViviCodeStatement *
parse_block (ParseData *data)
{
  ViviCodeStatement *statement;

  parse_token (data, TOKEN_BRACE_LEFT);

  if (!try_parse_token (data, TOKEN_BRACE_RIGHT)) {
    parse_statement_list (data, peek_statement, parse_statement, &statement,
	TOKEN_NONE);
    parse_token (data, TOKEN_BRACE_RIGHT);
  } else {
    statement = vivi_code_block_new ();;
  }

  return statement;
}

static gboolean
peek_variable_statement (ParseData *data)
{
  return peek_token (data, TOKEN_VAR);
}

static ViviCodeStatement *
parse_variable_statement (ParseData *data)
{
  ViviCodeStatement *statement;

  parse_token (data, TOKEN_VAR);
  parse_statement_list (data, peek_variable_declaration,
      parse_variable_declaration, &statement, TOKEN_COMMA);
  parse_automatic_semicolon (data);

  return statement;
}

static const struct {
  PeekFunction peek;
  ParseStatementFunction parse;
} statement_functions[] = {
  { peek_builtin_statement, parse_builtin_statement },
  { peek_block, parse_block },
  { peek_variable_statement, parse_variable_statement },
  { peek_empty_statement, parse_empty_statement },
  { peek_expression_statement, parse_expression_statement },
  { peek_if_statement, parse_if_statement },
  { peek_iteration_statement, parse_iteration_statement },
  { peek_continue_statement, parse_continue_statement },
  { peek_break_statement, parse_break_statement },
  { peek_return_statement, parse_return_statement },
  //{ peek_with_statement, parse_with_statement },
  //{ peek_switch_statement, parse_switch_statement },
  { peek_throw_statement, parse_throw_statement },
  //{ peek_try_statement, parse_try_statement },
  { NULL, NULL }
};

static gboolean
peek_statement (ParseData *data)
{
  guint i;

  for (i = 0; statement_functions[i].peek != NULL; i++) {
    if (statement_functions[i].peek (data))
      return TRUE;
  }

  return FALSE;
}

static ViviCodeStatement *
parse_statement (ParseData *data)
{
  guint i;

  for (i = 0; statement_functions[i].peek != NULL; i++) {
    if (statement_functions[i].peek (data)) {
      return statement_functions[i].parse (data);
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_STATEMENT);
  return vivi_compiler_empty_statement_new ();
}

// function

static gboolean
peek_function_definition (ParseData *data)
{
  return peek_token (data, TOKEN_FUNCTION);
}

static gboolean
peek_source_element (ParseData *data);

static ViviCodeStatement *
parse_source_element (ParseData *data);

static ViviCodeValue *
parse_function_definition (ParseData *data, gboolean name_required)
{
  ViviCodeValue *function, **arguments;
  ViviCodeStatement *body;
  const char *name;
  guint i;

  arguments = NULL;
  body = NULL;

  parse_token (data, TOKEN_FUNCTION);

  if (name_required) {
    name = g_strdup (parse_identifier_value (data));
    parse_token (data, TOKEN_PARENTHESIS_LEFT);
  } else {
    if (!try_parse_token (data, TOKEN_PARENTHESIS_LEFT)) {
      if (peek_identifier (data)) {
	name = g_strdup (parse_identifier_value (data));
	parse_token (data, TOKEN_PARENTHESIS_LEFT);
      } else {
	vivi_parser_error_unexpected_or (data, TOKEN_PARENTHESIS_LEFT,
	    ERROR_TOKEN_IDENTIFIER, TOKEN_NONE);
	name = NULL;
      }
    } else {
      name = NULL;
    }
  }

  if (!try_parse_token (data, TOKEN_PARENTHESIS_RIGHT)) {
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
  }

  parse_token (data, TOKEN_BRACE_LEFT);

  vivi_parser_start_level (data);
  parse_statement_list (data, peek_source_element, parse_source_element,
      &body, TOKEN_NONE);
  vivi_parser_end_level (data);

  parse_token (data, TOKEN_BRACE_RIGHT);

  function = vivi_code_function_new (name);
  if (body != NULL) {
    vivi_code_function_set_body (VIVI_CODE_FUNCTION (function), body);
    g_object_unref (body);
  }
  if (arguments != NULL) {
    for (i = 0; arguments[i] != NULL; i++) {
      vivi_code_function_add_argument (VIVI_CODE_FUNCTION (function),
	  vivi_code_constant_get_variable_name (VIVI_CODE_CONSTANT (
	      VIVI_CODE_GET (arguments[i])->name)));
    }
    free_value_list (arguments);
  }

  return function;
}

static gboolean
peek_function_declaration (ParseData *data)
{
  return peek_function_definition (data);
}

static ViviCodeStatement *
parse_function_declaration (ParseData *data)
{
  ViviCodeStatement *statement;
  ViviCodeValue *function;

  function = parse_function_definition (data, TRUE);
  statement = vivi_code_value_statement_new (function);
  g_object_unref (function);

  return statement;
}

static gboolean
peek_function_expression (ParseData *data)
{
  return peek_function_definition (data);
}

static ViviCodeValue *
parse_function_expression (ParseData *data)
{
  return parse_function_definition (data, FALSE);
}

// top

static gboolean
peek_source_element (ParseData *data)
{
  return (peek_function_declaration (data) || peek_statement (data));
}

static ViviCodeStatement *
parse_source_element (ParseData *data)
{
  if (peek_function_declaration (data)) {
    return parse_function_declaration (data);
  } else if (peek_statement (data)) {
    return parse_statement (data);
  } else {
    vivi_parser_error_unexpected_or (data, ERROR_TOKEN_FUNCTION_DECLARATION,
	ERROR_TOKEN_STATEMENT, TOKEN_NONE);
    return vivi_compiler_empty_statement_new ();
  }
}

G_GNUC_UNUSED static gboolean
peek_program (ParseData *data)
{
  return peek_source_element (data);
}

static ViviCodeStatement *
parse_program (ParseData *data)
{
  ViviCodeStatement *statement;

  g_assert (data->level == NULL);
  vivi_parser_start_level (data);

  parse_statement_list (data, peek_source_element, parse_source_element,
      &statement, TOKEN_NONE);
  parse_token (data, TOKEN_EOF);

  vivi_parser_end_level (data);
  g_assert (data->level == NULL);

  return statement;
}

// parsing

static void
parse_statement_list (ParseData *data, PeekFunction peek,
    ParseStatementFunction parse, ViviCodeStatement **block, guint separator)
{
  ViviCodeStatement *statement;

  g_assert (data != NULL);
  g_assert (peek != NULL);
  g_assert (parse != NULL);
  g_assert (block != NULL);

  *block = vivi_code_block_new ();

  do {
    statement = parse (data);
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (*block), statement);
    g_object_unref (statement);
  } while ((separator == TOKEN_NONE || try_parse_token (data, separator)) &&
      peek (data));
}

static void
parse_value_statement_list (ParseData *data, PeekFunction peek,
    ParseValueStatementFunction parse, ViviCodeValue ***list,
    ViviCodeStatement **statement, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;
  ViviCodeStatement *statement_one;

  g_assert (data != NULL);
  g_assert (peek != NULL);
  g_assert (parse != NULL);
  g_assert (list != NULL);
  g_assert (statement != NULL);

  *statement = NULL;

  array = g_ptr_array_new ();

  do {
    value = parse (data, &statement_one);
    g_ptr_array_add (array, value);
    *statement = vivi_parser_join_statements (*statement, statement_one);
  } while ((separator == TOKEN_NONE || try_parse_token (data, separator)) &&
      peek (data));

  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);
}

// public

ViviCodeStatement *
vivi_parse_file (FILE *file, const char *input_name)
{
  ParseData data;
  ViviCodeStatement *statement;

  g_return_val_if_fail (file != NULL, NULL);

  data.scanner = vivi_parser_scanner_new (file);
  vivi_parser_scanner_set_error_handler (data.scanner,
      vivi_parser_error_handler, &data);
  data.levels = NULL;
  data.level = NULL;
  data.error_count = 0;

  statement = parse_program (&data);
  if (data.error_count != 0) {
    g_object_unref (statement);
    statement = NULL;
  }

  g_object_unref (data.scanner);

  return statement;
}
