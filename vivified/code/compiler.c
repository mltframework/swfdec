#include <glib.h>
#include <string.h>

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
  SYMBOL_EXPRESSION,
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

typedef ParseStatus (*ParseFunction) (GScanner *scanner);

static ParseStatus parse (GScanner *scanner, ParseSymbol symbol);
static ParseStatus parse_list (GScanner *scanner, ParseSymbol symbol);

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

// top

static ParseStatus
parse_source_element (GScanner *scanner)
{
  ParseStatus status;

  status = parse (scanner, SYMBOL_FUNCTION_DECLARATION);

  if (status == STATUS_CANCEL)
    status = parse (scanner, SYMBOL_STATEMENT);

  return status;
}

// function

static ParseStatus
parse_function_declaration (GScanner *scanner)
{
  if (!check_token (scanner, TOKEN_FUNCTION))
    return STATUS_CANCEL;

  if (parse (scanner, SYMBOL_IDENTIFIER) != STATUS_OK)
    return STATUS_FAIL;

  if (!check_token (scanner, '('))
    return STATUS_FAIL;

  if (parse_list (scanner, SYMBOL_IDENTIFIER) == STATUS_FAIL)
    return STATUS_FAIL;

  if (!check_token (scanner, ')'))
    return STATUS_FAIL;

  if (!check_token (scanner, '{'))
    return STATUS_FAIL;

  if (parse_list (scanner, SYMBOL_SOURCE_ELEMENT) == STATUS_FAIL)
    return STATUS_FAIL;

  if (!check_token (scanner, '}'))
    return STATUS_FAIL;

  return STATUS_OK;
}

// statement

static ParseStatus
parse_statement (GScanner *scanner)
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

  for (i = 0; options[i] != SYMBOL_NONE; i++) {
    status = parse (scanner, options[i]);
    if (status != STATUS_CANCEL)
      return status;
  }

  return STATUS_CANCEL;
}

static ParseStatus
parse_block (GScanner *scanner)
{
  if (!check_token (scanner, '{'))
    return STATUS_CANCEL;

  g_scanner_peek_next_token (scanner);
  if (scanner->next_token != '}') {
    if (parse_list (scanner, SYMBOL_STATEMENT) != STATUS_OK)
      return STATUS_FAIL;
  }

  if (!check_token (scanner, '}'))
    return STATUS_FAIL;

  return STATUS_OK;
}

static ParseStatus
parse_empty_statement (GScanner *scanner)
{
  if (!check_token (scanner, ';'))
    return STATUS_CANCEL;

  return STATUS_OK;
}

static ParseStatus
parse_expression_statement (GScanner *scanner)
{
  g_scanner_peek_next_token (scanner);
  if (scanner->next_token == '{' || scanner->next_token == TOKEN_FUNCTION)
    return STATUS_CANCEL;

  return parse (scanner, SYMBOL_EXPRESSION);
}

// expression

static ParseStatus
parse_expression (GScanner *scanner)
{
  ParseStatus status;

  status = parse (scanner, SYMBOL_ASSIGNMENT_EXPRESSION);
  if (status != STATUS_OK)
    return status;

  do {
    if (!check_token (scanner, ','))
      return STATUS_OK;
  } while (parse (scanner, SYMBOL_ASSIGNMENT_EXPRESSION) == STATUS_OK);

  return STATUS_FAIL;
}

static ParseStatus
parse_assignment_expression (GScanner *scanner)
{
  ParseStatus status;

  status = parse (scanner, SYMBOL_LEFT_HAND_SIDE_EXPRESSION);
  if (status == STATUS_OK) {
    status = parse (scanner, SYMBOL_ASSIGNMENT_OPERATOR);
    if (status == STATUS_CANCEL)
      return STATUS_OK;
    if (status == STATUS_FAIL)
      return STATUS_FAIL;
    if (parse (scanner, SYMBOL_ASSIGNMENT_EXPRESSION) != STATUS_OK)
      return STATUS_FAIL;
    return STATUS_OK;
  } else if (status == STATUS_CANCEL) {
    return parse (scanner, SYMBOL_CONDITIONAL_EXPRESSION);
  } else {
    return STATUS_FAIL;
  }
}

static ParseStatus
parse_conditional_expression (GScanner *scanner)
{
  ParseStatus status;

  status = parse (scanner, SYMBOL_OPERATOR_EXPRESSION);
  if (status != STATUS_OK)
    return status;

  if (!check_token (scanner, '?'))
    return STATUS_OK;

  if (parse (scanner, SYMBOL_ASSIGNMENT_EXPRESSION) != STATUS_OK)
    return STATUS_FAIL;

  if (!check_token (scanner, ':'))
    return STATUS_FAIL;

  if (parse (scanner, SYMBOL_ASSIGNMENT_EXPRESSION) != STATUS_OK)
    return STATUS_FAIL;

  return STATUS_OK;
}

static ParseStatus
parse_operator_expression (GScanner *scanner)
{
  ParseStatus status;

  status = parse (scanner, SYMBOL_UNARY_EXPRESSION);
  if (status != STATUS_OK)
    return status;

  do {
    if (!check_token (scanner, '+'))
      return STATUS_OK;
  } while (parse (scanner, SYMBOL_UNARY_EXPRESSION) == STATUS_OK);

  return STATUS_FAIL;
}

static ParseStatus
parse_unary_expression (GScanner *scanner)
{
  ParseStatus status;

  status = parse (scanner, SYMBOL_POSTFIX_EXPRESSION);
  if (status != STATUS_OK)
    return status;

  do {
    if (!check_token (scanner, '!'))
      return STATUS_OK;
  } while (parse (scanner, SYMBOL_POSTFIX_EXPRESSION) == STATUS_OK);

  return STATUS_FAIL;
}

static ParseStatus
parse_postfix_expression (GScanner *scanner)
{
  ParseStatus status;

  status = parse (scanner, SYMBOL_LEFT_HAND_SIDE_EXPRESSION);
  if (status != STATUS_OK)
    return status;

  // don't allow new line here

  g_scanner_peek_next_token (scanner);
  if (scanner->next_token == TOKEN_PLUSPLUS ||
      scanner->next_token == TOKEN_MINUSMINUS) {
    g_scanner_get_next_token (scanner);
    return STATUS_OK;
  }

  return STATUS_OK;
}

static ParseStatus
parse_left_hand_side_expression (GScanner *scanner)
{
  ParseStatus status;

  status = parse (scanner, SYMBOL_NEW_EXPRESSION);
  if (status == STATUS_CANCEL)
    status = parse (scanner, SYMBOL_CALL_EXPRESSION);

  return status;
}

static ParseStatus
parse_new_expression (GScanner *scanner)
{
  g_scanner_peek_next_token (scanner);
  if (scanner->next_token == TOKEN_NEW) {
    g_scanner_get_next_token (scanner);
    if (parse (scanner, SYMBOL_NEW_EXPRESSION) != STATUS_OK)
      return STATUS_FAIL;
    return STATUS_OK;
  } else {
    return parse (scanner, SYMBOL_MEMBER_EXPRESSION);
  }
}

static ParseStatus
parse_member_expression (GScanner *scanner)
{
  ParseStatus status;

  g_scanner_peek_next_token (scanner);
  if (scanner->next_token == TOKEN_NEW) {
    g_scanner_get_next_token (scanner);
    if (parse (scanner, SYMBOL_MEMBER_EXPRESSION) != STATUS_OK)
      return STATUS_FAIL;
    if (parse (scanner, SYMBOL_ARGUMENTS) != STATUS_OK)
      return STATUS_FAIL;
    return STATUS_OK;
  }

  status = parse (scanner, SYMBOL_PRIMARY_EXPRESSION);
  if (status == STATUS_CANCEL)
    status = parse (scanner, SYMBOL_FUNCTION_EXPRESSION);

  if (status != 0)
    return status;

  do {
    g_scanner_peek_next_token (scanner);
    if (scanner->next_token == '[') {
      g_scanner_get_next_token (scanner);
      if (parse (scanner, SYMBOL_EXPRESSION) != STATUS_OK)
	return STATUS_FAIL;
      if (!check_token (scanner, ']'))
	return STATUS_FAIL;
    } else if (scanner->next_token == '.') {
      g_scanner_get_next_token (scanner);
      if (parse (scanner, SYMBOL_IDENTIFIER) != STATUS_OK)
	return STATUS_FAIL;
    } else {
      return STATUS_OK;
    }
  } while (TRUE);

  g_assert_not_reached ();
  return STATUS_FAIL;
}

static ParseStatus
parse_primary_expression (GScanner *scanner)
{
  return parse (scanner, SYMBOL_IDENTIFIER);
}

// misc.

static ParseStatus
parse_identifier (GScanner *scanner)
{
  if (!check_token (scanner, G_TOKEN_IDENTIFIER))
    return STATUS_CANCEL;

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

static int
parse (GScanner *scanner, ParseSymbol symbol)
{
  int i;

  for (i = 0; symbols[i].id != SYMBOL_NONE; i++) {
    if (symbols[i].id == symbol) {
      int ret = symbols[i].parse (scanner);
      if (ret != 1)
	g_print (":%i: %s\n", ret, symbols[i].name);
      return ret;
    }
  }

  //g_assert_not_reached ();
  return 1;
}

static gboolean
parse_list (GScanner *scanner, ParseSymbol symbol)
{
  int ret;

  ret = parse (scanner, symbol);
  if (ret != 0)
    return ret;

  do {
    ret = parse (scanner, symbol);
    if (ret == -1)
      return -1;
  } while (ret == 0);

  return 0;
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
    switch (scanner->token) {
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
