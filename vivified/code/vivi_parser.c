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

#include "vivi_code_and.h"
#include "vivi_code_assignment.h"
#include "vivi_code_binary_default.h"
#include "vivi_code_block.h"
#include "vivi_code_break.h"
#include "vivi_code_builtin_statement_default.h"
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
#include "vivi_code_or.h"
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
typedef void (*ParseValueFunction) (ParseData *data, ViviCodeValue **value);
typedef void (*ParseValueStatementFunction) (ParseData *data, ViviCodeValue **value, ViviCodeStatement **statement);
typedef void (*ParseStatementFunction) (ParseData *data, ViviCodeStatement **statement);

static void
parse_statement_list (ParseData *data, PeekFunction peek,
    ParseStatementFunction parse, ViviCodeStatement **block, guint separator);

static void
parse_value_statement_list (ParseData *data, PeekFunction peek,
    ParseValueStatementFunction parse, ViviCodeValue ***list,
    ViviCodeStatement **statement, guint separator);

static void
parse_value_list (ParseData *data, PeekFunction peek, ParseValueFunction parse,
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

  if (token != NULL)
    g_print (":: %i - %i\n", (int) start, (int) vivi_parser_get_position (data));
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
peek_undefined_literal (ParseData *data)
{
  return peek_token (data, TOKEN_UNDEFINED);
}

static void
parse_undefined_literal (ParseData *data, ViviCodeValue **value)
{
  vivi_parser_start_code_token (data);

  parse_token (data, TOKEN_UNDEFINED);

  *value = vivi_code_constant_new_undefined ();

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static gboolean
peek_null_literal (ParseData *data)
{
  return peek_token (data, TOKEN_NULL);
}

static void
parse_null_literal (ParseData *data, ViviCodeValue **value)
{
  vivi_parser_start_code_token (data);

  parse_token (data, TOKEN_NULL);

  *value = vivi_code_constant_new_null ();

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static gboolean
peek_boolean_literal (ParseData *data)
{
  return peek_token (data, TOKEN_BOOLEAN);
}

static void
parse_boolean_literal (ParseData *data, ViviCodeValue **value)
{
  vivi_parser_start_code_token (data);

  if (!try_parse_token (data, TOKEN_BOOLEAN)) {
    vivi_parser_error_unexpected (data, TOKEN_BOOLEAN);
    *value = vivi_code_constant_new_boolean (0);
  } else {
    *value = vivi_code_constant_new_boolean (data->scanner->value.v_boolean);
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static gboolean
peek_numeric_literal (ParseData *data)
{
  return peek_token (data, TOKEN_NUMBER);
}

static void
parse_numeric_literal (ParseData *data, ViviCodeValue **value)
{
  vivi_parser_start_code_token (data);

  if (!try_parse_token (data, TOKEN_NUMBER)) {
    vivi_parser_error_unexpected (data, TOKEN_NUMBER);
    *value = vivi_code_constant_new_number (0);
  } else {
    *value = vivi_code_constant_new_number (data->scanner->value.v_number);
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static gboolean
peek_string_literal (ParseData *data)
{
  return peek_token (data, TOKEN_STRING);
}

static void
parse_string_literal (ParseData *data, ViviCodeValue **value)
{
  vivi_parser_start_code_token (data);

  if (!try_parse_token (data, TOKEN_STRING)) {
    vivi_parser_error_unexpected (data, TOKEN_STRING);
    *value = vivi_code_constant_new_string ("undefined");
  } else {
    *value = vivi_code_constant_new_string (data->scanner->value.v_string);
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static const struct {
  PeekFunction peek;
  ParseValueFunction parse;
} literal_functions[] = {
  { peek_undefined_literal, parse_undefined_literal },
  { peek_null_literal, parse_null_literal },
  { peek_boolean_literal, parse_boolean_literal },
  { peek_numeric_literal, parse_numeric_literal },
  { peek_string_literal, parse_string_literal },
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

static void
parse_literal (ParseData *data, ViviCodeValue **value)
{
  guint i;

  for (i = 0; literal_functions[i].peek != NULL; i++) {
    if (literal_functions[i].peek (data)) {
      literal_functions[i].parse (data, value);
      return;
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_LITERAL);
  vivi_parser_start_code_token (data);
  *value = vivi_code_constant_new_undefined ();
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static gboolean
peek_identifier (ParseData *data)
{
  return peek_token (data, TOKEN_IDENTIFIER);
}

static void
parse_identifier (ParseData *data, ViviCodeValue **value)
{
  vivi_parser_start_code_token (data);

  if (!try_parse_token (data, TOKEN_IDENTIFIER)) {
    vivi_parser_error_unexpected (data, TOKEN_IDENTIFIER);
    *value = vivi_code_get_new_name ("undefined");
  } else {
    *value = vivi_code_get_new_name (data->scanner->value.v_identifier);
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static const struct {
  PeekFunction peek;
  ParseValueFunction parse;
} property_name_functions[] = {
  { peek_identifier, parse_identifier },
  { peek_string_literal, parse_string_literal },
  { peek_numeric_literal, parse_numeric_literal },
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

static void
parse_property_name (ParseData *data, ViviCodeValue **value)
{
  guint i;

  for (i = 0; property_name_functions[i].peek != NULL; i++) {
    if (property_name_functions[i].peek (data)) {
      property_name_functions[i].parse (data, value);
      return;
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_PROPERTY_NAME);
  vivi_parser_start_code_token (data);
  *value = vivi_code_constant_new_undefined ();
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static gboolean
peek_assignment_expression (ParseData *data);

static void
parse_assignment_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement);

static gboolean
peek_array_literal (ParseData *data)
{
  return peek_token (data, TOKEN_BRACKET_LEFT);
}

static void
parse_array_literal (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *member;
  ViviCodeStatement *statement_new;

  vivi_parser_start_code_token (data);

  *value = vivi_code_init_array_new ();
  *statement = NULL;

  parse_token (data, TOKEN_BRACKET_LEFT);

  while (TRUE) {
    if (try_parse_token (data, TOKEN_BRACKET_RIGHT)) {
      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (*value),
	 vivi_code_constant_new_undefined ());
      break;
    } else if (try_parse_token (data, TOKEN_COMMA)) {
      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (*value),
	 vivi_code_constant_new_undefined ());
    }
    else if (peek_assignment_expression (data))
    {
      parse_assignment_expression (data, &member, &statement_new);

      *statement = vivi_parser_join_statements (*statement, statement_new);

      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (*value),
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

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static gboolean
peek_object_literal (ParseData *data)
{
  return peek_token (data, TOKEN_BRACE_LEFT);
}

static void
parse_object_literal (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  vivi_parser_start_code_token (data);

  *value = vivi_code_init_object_new ();
  *statement = NULL;

  parse_token (data, TOKEN_BRACE_LEFT);

  if (!peek_token (data, TOKEN_BRACE_RIGHT)) {
    do {
      ViviCodeValue *property, *initializer;
      ViviCodeStatement *statement_new;

      parse_property_name (data, &property);
      parse_token (data, TOKEN_COLON);
      parse_assignment_expression (data, &initializer, &statement_new);

      *statement = vivi_parser_join_statements (*statement, statement_new);

      vivi_code_init_object_add_variable (VIVI_CODE_INIT_OBJECT (*value),
	  property, initializer);
    } while (try_parse_token (data, TOKEN_COMMA));
  }

  parse_token (data, TOKEN_BRACE_RIGHT);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

// misc

static gboolean
peek_variable_declaration (ParseData *data)
{
  return peek_identifier (data);
}

static void
parse_variable_declaration (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *identifier, *value;
  ViviCodeStatement *assignment, *statement_right;

  vivi_parser_start_code_token (data);

  parse_identifier (data, &identifier);

  if (try_parse_token (data, TOKEN_ASSIGN)) {
    parse_assignment_expression (data, &value, &statement_right);
  } else {
    vivi_parser_start_code_token (data);

    value = vivi_code_constant_new_undefined ();
    statement_right = NULL;

    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
  }

  assignment = vivi_parser_assignment_new (identifier, value);
  vivi_code_assignment_set_local (VIVI_CODE_ASSIGNMENT (assignment), TRUE);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (assignment));

  *statement = vivi_parser_join_statements (statement_right, assignment);
}

// special functions

typedef void (*ParseArgumentFunction) (ParseData *data, guint position,
    ViviCodeValue **value, ViviCodeStatement **statement);

/*static void
parse_url_method (ParseData *data, ViviCodeValue **value)
{
  if (peek_identifier (data)) {
    ViviCodeValue *identifier;
    const char *method;

    parse_identifier (data, &identifier);
    // FIXME
    method = vivi_code_constant_get_variable_name (VIVI_CODE_CONSTANT (
	  VIVI_CODE_GET (identifier)->name));
    if (g_ascii_strcasecmp (method, "GET") != 0 &&
	g_ascii_strcasecmp (method, "POST") != 0)
      vivi_parser_error (data, "Invalid URL method: %s\n", method);
    g_object_unref (identifier);
    *value = vivi_code_constant_new_string (method);
  } else if (peek_string_literal (data)) {
    parse_string_literal (data, value);
  } else {
    vivi_parser_error_unexpected_or (data, TOKEN_IDENTIFIER, TOKEN_STRING,
	TOKEN_NONE);
    *value = vivi_code_constant_new_string ("DEFAULT");
  }
}*/

typedef ViviCodeStatement *(*NewFunctionVoid) (void);
typedef ViviCodeStatement *(*NewFunctionValue) (ViviCodeValue *value);

typedef struct {
  gboolean			return_value;
  const char *			name;
  NewFunctionVoid		constructor_void;
  NewFunctionValue		constructor_value;
  ParseStatementFunction	parse_custom;
} SpecialFunction;

static const SpecialFunction special_functions[] = {
  //{ FALSE, "callFrame",          NULL, vivi_code_call_frame_new, NULL },
  //{ TRUE,  "chr",                NULL, vivi_code_chr_new, NULL },
  //{ TRUE,  "concat",             NULL, NULL, parse_concat },
  //{ FALSE, "duplicateMovieClip", NULL, NULL, parse_duplicate_movie_clip },
  //{ TRUE,  "eval",               NULL, vivi_code_eval_new, NULL },
  //{ TRUE,  "getProperty",        NULL, NULL, parse_get_property },
  //{ TRUE,  "getTimer",           vivi_code_get_timer_new, NULL, NULL },
  //{ FALSE, "getURL1",            NULL, NULL, parse_get_url1 },
  //{ FALSE, "getURL",             NULL, NULL, parse_get_url },
  //{ FALSE, "gotoAndPlay",        NULL, vivi_code_goto_and_play_new, NULL },
  //{ FALSE, "gotoAndStop",        NULL, vivi_code_goto_and_stop_new, NULL },
  //{ TRUE,  "int",                NULL, vivi_code_int_new, NULL },
  //{ TRUE,  "length",             NULL, vivi_code_length_new, NULL },
  //{ FALSE, "loadMovie",          NULL, NULL, parse_load_movie },
  //{ FALSE, "loadMovieNum",       NULL, NULL, parse_load_movie_num },
  //{ FALSE, "loadVariables",      NULL, NULL, parse_load_variables },
  //{ FALSE, "loadVariablesNum",   NULL, NULL, parse_load_variables_num },
  { FALSE, "nextFrame",          vivi_code_next_frame_new, NULL, NULL },
  //{ TRUE,  "ord",                NULL, vivi_code_ord_new, NULL },
  { FALSE, "play",               vivi_code_play_new, NULL, NULL },
  { FALSE, "prevFrame",          vivi_code_previous_frame_new, NULL, NULL },
  //{ TRUE,  "random",             NULL, vivi_code_random_new, NULL },
  //{ FALSE, "removeMovieClip",    NULL, vivi_code_remove_movie_clip_new, NULL },
  //{ FALSE, "setProperty",        NULL, NULL, parse_set_property },
  //{ FALSE, "setTarget",          NULL, vivi_code_set_target_new, NULL },
  //{ FALSE, "startDrag",          NULL, NULL, parse_start_drag },
  { FALSE, "stop",               vivi_code_stop_new, NULL, NULL },
  { FALSE, "stopDrag",           vivi_code_stop_drag_new,NULL,  NULL },
  { FALSE, "stopSounds",         vivi_code_stop_sounds_new, NULL, NULL },
  //{ TRUE,  "substring",          NULL, NULL, parse_substring },
  //{ TRUE,  "targetPath",         NULL, vivi_code_target_path_new, NULL },
  { FALSE, "toggleQuality",      vivi_code_toggle_quality_new, NULL, NULL },
  { FALSE, "trace",              NULL, vivi_code_trace_new, NULL }//,
  //{ TRUE,  "typeOf",             NULL, vivi_code_type_of_new, NULL }
};

static gboolean
peek_special_statement (ParseData *data)
{
  guint i;
  const char *identifier;

  if (!peek_token (data, TOKEN_IDENTIFIER))
    return FALSE;

  identifier = data->scanner->next_value.v_identifier;

  // TODO: Check that ( follows?

  for (i = 0; i < G_N_ELEMENTS (special_functions); i++) {
    if (special_functions[i].return_value == FALSE &&
	g_ascii_strcasecmp (identifier, special_functions[i].name) == 0)
      return TRUE;
  }

  return FALSE;
}

static void
parse_special_statement (ParseData *data, ViviCodeStatement **statement)
{
  guint i;
  const char *identifier;
  ViviCodeValue *value, *argument;
  ViviCodeStatement *argument_statement;

  parse_identifier (data, &value);
  identifier = vivi_code_constant_get_variable_name (VIVI_CODE_CONSTANT (
	VIVI_CODE_GET (value)->name));
  g_object_unref (value);

  for (i = 0; i < G_N_ELEMENTS (special_functions); i++) {
    if (special_functions[i].return_value == FALSE &&
        g_ascii_strcasecmp (identifier, special_functions[i].name) == 0)
      break;
  }
  if (i >= G_N_ELEMENTS (special_functions)) {
    vivi_parser_error (data, "Unknown special statement: %s", identifier);
    i = 0;
  }

  if (special_functions[i].parse_custom != NULL) {
    special_functions[i].parse_custom (data, statement);
    return;
  }
//
  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  if (special_functions[i].constructor_value != NULL)
    parse_assignment_expression (data, &argument, &argument_statement);

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  parse_automatic_semicolon (data);

  if (special_functions[i].constructor_value != NULL) {
    *statement = special_functions[i].constructor_value (argument);
    g_object_unref (argument);
    *statement = vivi_parser_join_statements (argument_statement, *statement);
  } else {
    g_assert (special_functions[i].constructor_void != NULL);
    *statement = special_functions[i].constructor_void ();
  }
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

static void
parse_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement);

static void
parse_primary_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  guint i;

  vivi_parser_start_code_token (data);

  if (try_parse_token (data, TOKEN_THIS)) {
    *value = vivi_code_get_new_name ("this");
    *statement = NULL;

    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
    return;
  }

  if (try_parse_token (data, TOKEN_PARENTHESIS_LEFT)) {
    parse_expression (data, value, statement);
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);

    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
    return;
  }

  for (i = 0; primary_expression_functions[i].peek != NULL; i++) {
    if (primary_expression_functions[i].peek (data)) {
      if (primary_expression_functions[i].parse_value != NULL) {
	primary_expression_functions[i].parse_value (data, value);
	*statement = NULL;
      } else {
	primary_expression_functions[i].parse_value_statement (data, value,
	    statement);
      }
      vivi_parser_end_code_token (data, NULL);
      return;
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_PRIMARY_EXPRESSION);
  *value = vivi_code_constant_new_undefined ();
  *statement = NULL;
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
}

static gboolean
peek_function_expression (ParseData *data);

static void
parse_function_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement);

static gboolean
peek_member_expression (ParseData *data)
{
  return (peek_primary_expression (data) || peek_function_expression (data));
}

static void
parse_member_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *member;
  ViviCodeStatement *statement_member;

  vivi_parser_start_code_token (data);

  if (peek_primary_expression (data)) {
    parse_primary_expression (data, value, statement);
  } else if (peek_function_expression (data)) {
    parse_function_expression (data, value, statement);
  } else {
    vivi_parser_error_unexpected_or (data, ERROR_TOKEN_PRIMARY_EXPRESSION,
	ERROR_TOKEN_FUNCTION_EXPRESSION, TOKEN_NONE);
    *value = vivi_code_constant_new_undefined ();
    *statement = NULL;

    vivi_parser_duplicate_code_token (data);
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
  }

  while (TRUE) {
    ViviCodeValue *tmp;

    if (try_parse_token (data, TOKEN_BRACKET_LEFT)) {
      parse_expression (data, &member, &statement_member);

      *statement = vivi_parser_join_statements (*statement, statement_member);

      parse_token (data, TOKEN_BRACKET_RIGHT);
    } else if (try_parse_token (data, TOKEN_DOT)) {
      parse_identifier (data, &member);
    } else {
      vivi_parser_end_code_token (data, NULL);
      return;
    }

    tmp = *value;
    *value = vivi_parser_get_new (tmp, member);
    g_object_unref (tmp);
    g_object_unref (member);

    vivi_parser_duplicate_code_token (data);
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
  }

  g_assert_not_reached ();
}

static gboolean
peek_left_hand_side_expression (ParseData *data)
{
  return (peek_token (data, TOKEN_NEW) || peek_member_expression (data));

}

static void
parse_left_hand_side_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *name;
  ViviCodeValue **arguments;
  ViviCodeStatement *argument_statement;
  guint i;

  vivi_parser_start_code_token (data);

  if (try_parse_token (data, TOKEN_NEW)) {
    parse_left_hand_side_expression (data, value, statement);

    if (!VIVI_IS_CODE_FUNCTION_CALL (*value)) {
      ViviCodeValue *tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_parser_function_call_new (tmp);
      g_object_unref (tmp);
    }

    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (*value),
	TRUE);

    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
    return;
  }

  parse_member_expression (data, value, statement);

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

    vivi_parser_duplicate_code_token (data);
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));
  }

  vivi_parser_end_code_token (data, NULL);
}

static gboolean
peek_postfix_expression (ParseData *data)
{
  return peek_left_hand_side_expression (data);
}

static void
parse_postfix_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeStatement *assignment;
  ViviCodeValue *operation, *one, *temporary;
  gboolean add;

  vivi_parser_start_code_token (data);

  parse_left_hand_side_expression (data, value, statement);

  if (peek_line_terminator (data)) {
    vivi_parser_end_code_token (data, NULL);
    return;
  }

  if (try_parse_token (data, TOKEN_INCREASE)) {
    add = TRUE;
  } else if (try_parse_token (data, TOKEN_DESCREASE)) {
    add = FALSE;
  } else {
    vivi_parser_end_code_token (data, NULL);
    return;
  }

  // FIXME: Correct?
  if (!vivi_parser_value_is_left_hand_side (*value)) {
    vivi_parser_error (data,
	"Invalid left-hand side expression for INCREASE/DECREASE");
  }

  one = vivi_code_constant_new_number (1);
  operation = (add ? vivi_code_add_new : vivi_code_subtract_new) (*value, one);
  g_object_unref (one);

  vivi_parser_duplicate_code_token (data);
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (operation));

  assignment = vivi_parser_assignment_new (*value, operation);
  g_object_unref (operation);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (assignment));

  temporary = vivi_compiler_get_temporary_new ();
  *statement = vivi_parser_join_statements (*statement,
     vivi_parser_join_statements (
       vivi_parser_assignment_new (temporary, *value), assignment));

  g_object_unref (*value);
  *value = temporary;
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

static void
parse_unary_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *tmp, *one;

  vivi_parser_scanner_peek_next_token (data->scanner);
  switch ((guint)data->scanner->next_token) {
    /*case TOKEN_DELETE:
    case TOKEN_VOID:
    case TOKEN_TYPEOF:*/
    case TOKEN_INCREASE:
    case TOKEN_DESCREASE:
      vivi_parser_start_code_token (data);

      vivi_parser_scanner_get_next_token (data->scanner);

      parse_unary_expression (data, value, statement);

      // FIXME: Correct?
      if (!vivi_parser_value_is_left_hand_side (*value)) {
	vivi_parser_error (data,
	    "Invalid left-hand side expression for INCREASE/DECREASE");
      }

      one = vivi_code_constant_new_number (1);
      tmp = (data->scanner->next_token == TOKEN_INCREASE ? vivi_code_add_new : vivi_code_subtract_new) (*value, one);
      g_object_unref (one);

      vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (tmp));

      *statement = vivi_parser_join_statements (*statement,
	  vivi_parser_assignment_new (*value, tmp));
      g_object_unref (tmp);
      break;
    /*case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_BITWISE_NOT:*/
    case TOKEN_LOGICAL_NOT:
      vivi_parser_start_code_token (data);

      vivi_parser_scanner_get_next_token (data->scanner);

      parse_unary_expression (data, value, statement);

      tmp = VIVI_CODE_VALUE (*value);
      *value = vivi_code_unary_new (tmp, '!');
      g_object_unref (tmp);

      vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (tmp));
      break;
    default:
      parse_postfix_expression (data, value, statement);
      break;
  }
}

typedef enum {
  PASS_ALWAYS,
  PASS_LOGICAL_OR,
  PASS_LOGICAL_AND
} ParseOperatorPass;

static void
parse_operator_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement, const ViviParserScannerToken *tokens,
    ParseOperatorPass pass, ParseValueStatementFunction next_parse_function)
{
  ViviCodeValue *left, *right;
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
    { TOKEN_BITWISE_AND, vivi_code_bitwise_and_new },
    { TOKEN_BITWISE_OR, vivi_code_bitwise_or_new },
    { TOKEN_BITWISE_XOR, vivi_code_bitwise_xor_new },
    { TOKEN_SHIFT_LEFT, vivi_code_left_shift_new },
    { TOKEN_SHIFT_RIGHT, vivi_code_right_shift_new },
    { TOKEN_SHIFT_RIGHT_UNSIGNED, vivi_code_unsigned_right_shift_new },
    { TOKEN_STRICT_EQUAL, vivi_code_strict_equals_new },
    { TOKEN_GREATER_THAN, vivi_code_greater_new },
//    { TOKEN_, vivi_code_string_greater_new },
    { TOKEN_LOGICAL_AND, vivi_code_and_new },
    { TOKEN_LOGICAL_OR, vivi_code_or_new }
  };

  vivi_parser_start_code_token (data);

  next_parse_function (data, value, statement);

again:
  for (i = 0; tokens[i] != TOKEN_NONE; i++) {
    if (try_parse_token (data, tokens[i])) {
      next_parse_function (data, &right, &statement_right);

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
      for (j = 0; j < G_N_ELEMENTS (table); j++) {
	if (tokens[i] != table[j].token)
	  continue;

	*value = table[j].func (left, VIVI_CODE_VALUE (right));
	g_object_unref (left);
	g_object_unref (right);

	vivi_parser_duplicate_code_token (data);
	vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*value));

	goto again;
      }
      g_assert_not_reached ();
    }
  }

  vivi_parser_end_code_token (data, NULL);
}

static gboolean
peek_multiplicative_expression (ParseData *data)
{
  return peek_unary_expression (data);
}

static void
parse_multiplicative_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_MULTIPLY,
    TOKEN_DIVIDE, TOKEN_REMAINDER, TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_ALWAYS,
      parse_unary_expression);
}

static gboolean
peek_additive_expression (ParseData *data)
{
  return peek_multiplicative_expression (data);
}

static void
parse_additive_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_ALWAYS,
      parse_multiplicative_expression);
}

static gboolean
peek_shift_expression (ParseData *data)
{
  return peek_additive_expression (data);
}

static void
parse_shift_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT, TOKEN_SHIFT_RIGHT_UNSIGNED, TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_ALWAYS,
      parse_additive_expression);
}

static gboolean
peek_relational_expression (ParseData *data)
{
  return peek_shift_expression (data);
}

static void
parse_relational_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN, /*TOKEN_LESS_THAN_OR_EQUAL,
    TOKEN_EQUAL_OR_GREATER_THAN, TOKEN_INSTANCEOF, TOKEN_IN,*/ TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_ALWAYS,
      parse_shift_expression);
}

static gboolean
peek_equality_expression (ParseData *data)
{
  return peek_relational_expression (data);
}

static void
parse_equality_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_EQUAL,
    /*TOKEN_NOT_EQUAL,*/ TOKEN_STRICT_EQUAL, /*TOKEN_NOT_STRICT_EQUAL,*/
    TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_ALWAYS,
      parse_relational_expression);
}

static gboolean
peek_bitwise_and_expression (ParseData *data)
{
  return peek_equality_expression (data);
}

static void
parse_bitwise_and_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_AND,
    TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_ALWAYS,
      parse_equality_expression);
}

static gboolean
peek_bitwise_xor_expression (ParseData *data)
{
  return peek_bitwise_and_expression (data);
}

static void
parse_bitwise_xor_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_XOR,
    TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_ALWAYS,
      parse_bitwise_and_expression);
}

static gboolean
peek_bitwise_or_expression (ParseData *data)
{
  return peek_bitwise_xor_expression (data);
}

static void
parse_bitwise_or_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_OR,
    TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_ALWAYS,
      parse_bitwise_xor_expression);
}

static gboolean
peek_logical_and_expression (ParseData *data)
{
  return peek_bitwise_or_expression (data);
}

static void
parse_logical_and_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LOGICAL_AND,
    TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_LOGICAL_AND,
      parse_bitwise_or_expression);
}

static gboolean
peek_logical_or_expression (ParseData *data)
{
  return peek_logical_and_expression (data);
}

static void
parse_logical_or_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LOGICAL_OR,
    TOKEN_NONE };

  parse_operator_expression (data, value, statement, tokens, PASS_LOGICAL_OR,
      parse_logical_and_expression);
}

static gboolean
peek_conditional_expression (ParseData *data)
{
  return peek_logical_or_expression (data);
}

static void
parse_conditional_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  //ViviCodeStatement *if_statement, *else_statement;

  parse_logical_or_expression (data, value, statement);

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

static void
parse_assignment_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *right;
  ViviCodeStatement *assignment, *statement_right;
  ViviCodeValue * (* func) (ViviCodeValue *, ViviCodeValue *);

  vivi_parser_start_code_token (data);

  parse_conditional_expression (data, value, statement);

  // FIXME: Correct?
  if (!vivi_parser_value_is_left_hand_side (*value)) {
    vivi_parser_end_code_token (data, NULL);
    return;
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
    case TOKEN_ASSIGN_SHIFT_LEFT:
      func = vivi_code_left_shift_new;
      break;
    case TOKEN_ASSIGN_SHIFT_RIGHT:
      func = vivi_code_right_shift_new;
      break;
    case TOKEN_ASSIGN_SHIFT_RIGHT_ZERO:
      func = vivi_code_unsigned_right_shift_new;
      break;
    case TOKEN_ASSIGN_BITWISE_AND:
      func = vivi_code_bitwise_and_new;
      break;
    case TOKEN_ASSIGN_BITWISE_OR:
      func = vivi_code_bitwise_or_new;
      break;
    case TOKEN_ASSIGN_BITWISE_XOR:
      func = vivi_code_bitwise_and_new;
      break;
    case TOKEN_ASSIGN:
      func = NULL;
      break;
    default:
      return;
  }
  
  vivi_parser_scanner_get_next_token (data->scanner);

  parse_assignment_expression (data, &right, &statement_right);

  if (func != NULL) {
    assignment = vivi_parser_assignment_new (*value,
	func (*value, right));
  } else {
    assignment = vivi_parser_assignment_new (*value, right);
  }
  g_object_unref (right);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (assignment));

  *statement = vivi_parser_join_statements (*statement,
      vivi_parser_join_statements (statement_right, assignment));
}

static gboolean
peek_expression (ParseData *data)
{
  return peek_assignment_expression (data);
}

static void
parse_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeStatement *statement_one;

  *value = NULL;
  *statement = NULL;

  do {
    if (*value != NULL)
      g_object_unref (*value);
    parse_assignment_expression (data, value, &statement_one);
    *statement = vivi_parser_join_statements (*statement, statement_one);
  } while (try_parse_token (data, TOKEN_COMMA));
}

// statement

static void
parse_continue_or_break_statement (ParseData *data,
    ViviCodeStatement **statement, ViviParserScannerToken token)
{
  vivi_parser_start_code_token (data);

  parse_token (data, token);

  if (!try_parse_restricted_semicolon (data)) {
    ViviCodeValue *identifier;

    parse_identifier (data, &identifier);

    // FIXME
    *statement = vivi_compiler_goto_name_new (
	vivi_code_constant_get_variable_name (
	  VIVI_CODE_CONSTANT (VIVI_CODE_GET (identifier)->name)));
    g_object_unref (identifier);

    vivi_parser_add_goto (data, VIVI_COMPILER_GOTO_NAME (*statement));

    parse_automatic_semicolon (data);
  } else {
    // FIXME
    *statement = vivi_code_break_new ();
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (*statement));
}

static gboolean
peek_continue_statement (ParseData *data)
{
  return peek_token (data, TOKEN_CONTINUE);
}

static void
parse_continue_statement (ParseData *data, ViviCodeStatement **statement)
{
  return parse_continue_or_break_statement (data, statement, TOKEN_CONTINUE);
}

static gboolean
peek_break_statement (ParseData *data)
{
  return peek_token (data, TOKEN_BREAK);
}

static void
parse_break_statement (ParseData *data, ViviCodeStatement **statement)
{
  parse_continue_or_break_statement (data, statement, TOKEN_BREAK);
}

static gboolean
peek_throw_statement (ParseData *data)
{
  return peek_token (data, TOKEN_THROW);
}

static void
parse_throw_statement (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value;
  ViviCodeStatement *expression_statement;

  parse_token (data, TOKEN_THROW);

  if (peek_line_terminator (data)) {
    vivi_parser_error_unexpected_line_terminator (data,
	ERROR_TOKEN_EXPRESSION);
  }

  parse_expression (data, &value, &expression_statement);

  *statement = vivi_code_throw_new (value);
  g_object_unref (value);

  *statement = vivi_parser_join_statements (expression_statement, *statement);

  parse_automatic_semicolon (data);
}

static gboolean
peek_return_statement (ParseData *data)
{
  return peek_token (data, TOKEN_RETURN);
}

static void
parse_return_statement (ParseData *data, ViviCodeStatement **statement)
{
  *statement = vivi_code_return_new ();

  parse_token (data, TOKEN_RETURN);

  if (!try_parse_restricted_semicolon (data)) {
    ViviCodeValue *value;
    ViviCodeStatement *expression_statement;

    parse_expression (data, &value, &expression_statement);

    vivi_code_return_set_value (VIVI_CODE_RETURN (*statement), value);
    g_object_unref (value);

    *statement =
      vivi_parser_join_statements (expression_statement, *statement);

    parse_automatic_semicolon (data);
  }
}

static gboolean
peek_statement (ParseData *data);

static void
parse_statement (ParseData *data, ViviCodeStatement **statement);

static gboolean
peek_iteration_statement (ParseData *data)
{
  return (peek_token (data, TOKEN_DO) || peek_token (data, TOKEN_WHILE) ||
      peek_token (data, TOKEN_FOR));
}

static void
parse_iteration_statement (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *condition;
  ViviCodeStatement *pre_statement, *condition_statement, *loop_statement;

  if (try_parse_token (data, TOKEN_DO))
  {
    parse_statement (data, &loop_statement);
    parse_token (data, TOKEN_WHILE);
    parse_token (data, TOKEN_PARENTHESIS_LEFT);
    parse_expression (data, &condition, &condition_statement);
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    parse_automatic_semicolon (data);

    pre_statement = g_object_ref (loop_statement);
  }
  else if (try_parse_token (data, TOKEN_WHILE))
  {
    parse_token (data, TOKEN_PARENTHESIS_LEFT);
    parse_expression (data, &condition, &condition_statement);
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    parse_statement (data, &loop_statement);

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
	parse_expression (data, &pre_value, &pre_statement);
      }
    }

    if (try_parse_token (data, TOKEN_SEMICOLON)) {
      if (pre_value != NULL)
	g_object_unref (pre_value);

      if (try_parse_token (data, TOKEN_SEMICOLON)) {
	condition = vivi_code_constant_new_boolean (TRUE);
	condition_statement = NULL;
      } else {
	parse_expression (data, &condition, &condition_statement);
	parse_token (data, TOKEN_SEMICOLON);
      }

      if (peek_token (data, TOKEN_PARENTHESIS_RIGHT)) {
	post_statement = NULL;
      } else {
	parse_expression (data, &pre_value, &post_statement);
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

      condition = vivi_code_constant_new_undefined ();
      condition_statement = NULL;
      post_statement = NULL;
    } else {
      if (pre_value != NULL) {
	vivi_parser_error_unexpected_or (data, TOKEN_SEMICOLON, TOKEN_IN,
	    TOKEN_NONE);
      } else {
	vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);
      }

      condition = vivi_code_constant_new_undefined ();
      condition_statement = NULL;
      post_statement = NULL;

      if (pre_value != NULL)
	g_object_unref (pre_value);
      if (pre_statement != NULL)
	g_object_unref (pre_statement);
    }

    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    parse_statement (data, &loop_statement);

    loop_statement =
      vivi_parser_join_statements (loop_statement, post_statement);
  }
  else
  {
    vivi_parser_error_unexpected (data, ERROR_TOKEN_ITERATION_STATEMENT);

    condition = vivi_code_constant_new_undefined ();
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

  *statement = vivi_code_loop_new ();
  vivi_code_loop_set_condition (VIVI_CODE_LOOP (*statement), condition);
  g_object_unref (condition);
  vivi_code_loop_set_statement (VIVI_CODE_LOOP (*statement), loop_statement);
  g_object_unref (loop_statement);

  *statement = vivi_parser_join_statements (pre_statement, *statement);
}

static gboolean
peek_if_statement (ParseData *data)
{
  return peek_token (data, TOKEN_IF);
}

static void
parse_if_statement (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *condition;
  ViviCodeStatement *pre_statement, *if_statement, *else_statement;

  parse_token (data, TOKEN_IF);
  parse_token (data, TOKEN_PARENTHESIS_LEFT);
  parse_expression (data, &condition, &pre_statement);
  parse_token (data, TOKEN_PARENTHESIS_RIGHT);
  parse_statement (data, &if_statement);

  if (try_parse_token (data, TOKEN_ELSE)) {
    parse_statement (data, &else_statement);
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
}

static gboolean
peek_expression_statement (ParseData *data)
{
  if (peek_token (data, TOKEN_BRACE_LEFT) || peek_token (data, TOKEN_FUNCTION))
    return FALSE;

  return peek_expression (data);
}

static void
parse_expression_statement (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *value;
  ViviCodeStatement *last;

  if (peek_token (data, TOKEN_BRACE_LEFT) || peek_token (data, TOKEN_FUNCTION))
    vivi_parser_error_unexpected (data, ERROR_TOKEN_EXPRESSION_STATEMENT);

  parse_expression (data, &value, statement);

  // check for label
  if (*statement == NULL && vivi_parser_value_is_identifier (value) &&
      try_parse_token (data, TOKEN_COLON))
  {
    *statement = vivi_code_label_new (vivi_code_constant_get_variable_name (
	  VIVI_CODE_CONSTANT (VIVI_CODE_GET (value)->name)));
    if (!vivi_parser_add_label (data, VIVI_CODE_LABEL (*statement)))
      vivi_parser_error (data, "Same label name used twice");
    return;
  }

  parse_automatic_semicolon (data);

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
      return;
    }
  }

  *statement = vivi_parser_join_statements (*statement,
      vivi_code_value_statement_new (value));
  g_object_unref (value);
}

static gboolean
peek_empty_statement (ParseData *data)
{
  return peek_token (data, TOKEN_SEMICOLON);
}

static void
parse_empty_statement (ParseData *data, ViviCodeStatement **statement)
{
  parse_token (data, TOKEN_SEMICOLON);

  *statement = vivi_compiler_empty_statement_new ();
}

static gboolean
peek_block (ParseData *data)
{
  return peek_token (data, TOKEN_BRACE_LEFT);
}

static void
parse_block (ParseData *data, ViviCodeStatement **statement)
{
  parse_token (data, TOKEN_BRACE_LEFT);

  if (!try_parse_token (data, TOKEN_BRACE_RIGHT)) {
    parse_statement_list (data, peek_statement, parse_statement, statement,
	TOKEN_NONE);
    parse_token (data, TOKEN_BRACE_RIGHT);
  } else {
    *statement = vivi_code_block_new ();;
  }
}

static gboolean
peek_variable_statement (ParseData *data)
{
  return peek_token (data, TOKEN_VAR);
}

static void
parse_variable_statement (ParseData *data, ViviCodeStatement **statement)
{
  parse_token (data, TOKEN_VAR);
  parse_statement_list (data, peek_variable_declaration,
      parse_variable_declaration, statement, TOKEN_COMMA);
  parse_automatic_semicolon (data);
}

static const struct {
  PeekFunction peek;
  ParseStatementFunction parse;
} statement_functions[] = {
  { peek_special_statement, parse_special_statement },
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

static void
parse_statement (ParseData *data, ViviCodeStatement **statement)
{
  guint i;

  for (i = 0; statement_functions[i].peek != NULL; i++) {
    if (statement_functions[i].peek (data)) {
      statement_functions[i].parse (data, statement);
      return;
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_STATEMENT);
  *statement = vivi_compiler_empty_statement_new ();
}

// function

static gboolean
peek_function_definition (ParseData *data)
{
  return peek_token (data, TOKEN_FUNCTION);
}

static gboolean
peek_source_element (ParseData *data);

static void
parse_source_element (ParseData *data, ViviCodeStatement **statement);

static void
parse_function_definition (ParseData *data, ViviCodeValue **function,
    ViviCodeValue **identifier, gboolean identifier_required)
{
  ViviCodeValue **arguments;
  ViviCodeStatement *body;
  guint i;

  arguments = NULL;
  body = NULL;

  parse_token (data, TOKEN_FUNCTION);

  if (identifier_required) {
    parse_identifier (data, identifier);
    parse_token (data, TOKEN_PARENTHESIS_LEFT);
  } else {
    if (!try_parse_token (data, TOKEN_PARENTHESIS_LEFT)) {
      if (peek_identifier (data)) {
	parse_identifier (data, identifier);
	parse_token (data, TOKEN_PARENTHESIS_LEFT);
      } else {
	vivi_parser_error_unexpected_or (data, TOKEN_PARENTHESIS_LEFT,
	    ERROR_TOKEN_IDENTIFIER, TOKEN_NONE);
	*identifier = NULL;
      }
    } else {
      *identifier = NULL;
    }
  }

  if (!try_parse_token (data, TOKEN_PARENTHESIS_RIGHT)) {
    parse_value_list (data, peek_identifier, parse_identifier, &arguments,
	TOKEN_COMMA);
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
  }

  parse_token (data, TOKEN_BRACE_LEFT);

  vivi_parser_start_level (data);
  parse_statement_list (data, peek_source_element, parse_source_element,
      &body, TOKEN_NONE);
  vivi_parser_end_level (data);

  parse_token (data, TOKEN_BRACE_RIGHT);

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
}

static gboolean
peek_function_declaration (ParseData *data)
{
  return peek_function_definition (data);
}

static void
parse_function_declaration (ParseData *data, ViviCodeStatement **statement)
{
  ViviCodeValue *function, *identifier;

  parse_function_definition (data, &function, &identifier, TRUE);

  // FIXME
  *statement = vivi_parser_assignment_new (identifier, function);
  g_object_unref (identifier);
  g_object_unref (function);
}

static gboolean
peek_function_expression (ParseData *data)
{
  return peek_function_definition (data);
}

static void
parse_function_expression (ParseData *data, ViviCodeValue **value,
    ViviCodeStatement **statement)
{
  ViviCodeValue *function, *identifier;

  parse_function_definition (data, &function, &identifier, FALSE);

  // FIXME
  if (identifier != NULL) {
    *statement = vivi_parser_assignment_new (identifier, *value);
    g_object_unref (identifier);
  }
}

// top

static gboolean
peek_source_element (ParseData *data)
{
  return (peek_function_declaration (data) || peek_statement (data));
}

static void
parse_source_element (ParseData *data, ViviCodeStatement **statement)
{
  if (peek_function_declaration (data)) {
    parse_function_declaration (data, statement);
  } else if (peek_statement (data)) {
    parse_statement (data, statement);
  } else {
    vivi_parser_error_unexpected_or (data, ERROR_TOKEN_FUNCTION_DECLARATION,
	ERROR_TOKEN_STATEMENT, TOKEN_NONE);
    *statement = vivi_compiler_empty_statement_new ();
  }
}

G_GNUC_UNUSED static gboolean
peek_program (ParseData *data)
{
  return peek_source_element (data);
}

static void
parse_program (ParseData *data, ViviCodeStatement **statement)
{
  g_assert (data->level == NULL);
  vivi_parser_start_level (data);

  parse_statement_list (data, peek_source_element, parse_source_element,
      statement, TOKEN_NONE);
  parse_token (data, TOKEN_EOF);

  vivi_parser_end_level (data);
  g_assert (data->level == NULL);
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
    parse (data, &statement);
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
    parse (data, &value, &statement_one);
    g_ptr_array_add (array, value);
    *statement = vivi_parser_join_statements (*statement, statement_one);
  } while ((separator == TOKEN_NONE || try_parse_token (data, separator)) &&
      peek (data));

  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);
}

static void
parse_value_list (ParseData *data, PeekFunction peek, ParseValueFunction parse,
    ViviCodeValue ***list, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;

  g_assert (data != NULL);
  g_assert (peek != NULL);
  g_assert (parse != NULL);
  g_assert (list != NULL);

  array = g_ptr_array_new ();

  do {
    parse (data, &value);
    g_ptr_array_add (array, value);
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

  parse_program (&data, &statement);
  if (data.error_count != 0) {
    g_object_unref (statement);
    statement = NULL;
  }

  g_object_unref (data.scanner);

  return statement;
}
