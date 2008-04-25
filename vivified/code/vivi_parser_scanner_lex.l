%top{
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

#include "vivi_parser_scanner.h"

#define YY_DECL int yylex (ViviParserValue *value)
extern YY_DECL;
}

%{
guint lex_line_number = 0;
gsize lex_position = 0;

static void new_line (void);
static void count (void);

static GString *string = NULL;
%}

%option			noyywrap
%option			nostdinit
%option			never-interactive
%option			outfile="vivi_parser_scanner_lex.c"
%option			header-file="vivi_parser_scanner_lex.h"

identifier_start	[$_a-zA-Z]
identifier_part		[$_a-zA-Z0-9]

%x			str
%x			comment

%%

"/*"			{ count (); BEGIN(comment); }
<comment>{
  [^*\n]*		{ count (); /* skip */ }
  \n			{ count (); new_line (); }
  "*/"			{ count (); BEGIN(INITIAL); }
  <<EOF>>		{
			  count ();
			  BEGIN(INITIAL);
			  value->type = VALUE_TYPE_ERROR;
			  value->value.v_error =
			    g_strdup ("Unterminated comment");
			  return TOKEN_ERROR;
			}
}

"//"[^\r\n]*		{
			  count ();
			  new_line ();
			  return TOKEN_LINE_TERMINATOR;
			}

<<EOF>>			{ return TOKEN_EOF; }

[ \t]			{ count (); /* skip */ }

\r\n			{
			  count ();
			  new_line ();
			  return TOKEN_LINE_TERMINATOR;
			}
[\r\n]			{
			  count ();
			  new_line ();
			  return TOKEN_LINE_TERMINATOR;
			}

"{"			{ count (); return TOKEN_BRACE_LEFT; }
"}"			{ count (); return TOKEN_BRACE_RIGHT; }
"["			{ count (); return TOKEN_BRACKET_LEFT; }
"]"			{ count (); return TOKEN_BRACKET_RIGHT; }
"("			{ count (); return TOKEN_PARENTHESIS_LEFT; }
")"			{ count (); return TOKEN_PARENTHESIS_RIGHT; }

"."			{ count (); return TOKEN_DOT; }
";"			{ count (); return TOKEN_SEMICOLON; }
","			{ count (); return TOKEN_COMMA; }

"<"			{ count (); return TOKEN_LESS_THAN; }
">"			{ count (); return TOKEN_GREATER_THAN; }
"<="			{ count (); return TOKEN_LESS_THAN_OR_EQUAL; }
">="			{ count (); return TOKEN_EQUAL_OR_GREATER_THAN; }

"=="			{ count (); return TOKEN_EQUAL; }
"!="			{ count (); return TOKEN_NOT_EQUAL; }
"==="			{ count (); return TOKEN_STRICT_EQUAL; }
"!=="			{ count (); return TOKEN_NOT_STRICT_EQUAL; }

"+"			{ count (); return TOKEN_PLUS; }
"-"			{ count (); return TOKEN_MINUS; }
"*"			{ count (); return TOKEN_MULTIPLY; }
"/"			{ count (); return TOKEN_DIVIDE; }
"%"			{ count (); return TOKEN_REMAINDER; }

"<<"			{ count (); return TOKEN_SHIFT_LEFT; }
">>"			{ count (); return TOKEN_SHIFT_RIGHT; }
">>>"			{ count (); return TOKEN_SHIFT_RIGHT_UNSIGNED; }

"&"			{ count (); return TOKEN_BITWISE_AND; }
"|"			{ count (); return TOKEN_BITWISE_OR; }
"^"			{ count (); return TOKEN_BITWISE_XOR; }

"!"			{ count (); return TOKEN_LOGICAL_NOT; }
"~"			{ count (); return TOKEN_BITWISE_NOT; }
"++"			{ count (); return TOKEN_INCREASE; }
"--"			{ count (); return TOKEN_DESCREASE; }

"?"			{ count (); return TOKEN_QUESTION_MARK; }
":"			{ count (); return TOKEN_COLON; }

"&&"			{ count (); return TOKEN_LOGICAL_AND; }
"||"			{ count (); return TOKEN_LOGICAL_OR; }

"="			{ count (); return TOKEN_ASSIGN; }
"*="			{ count (); return TOKEN_ASSIGN_MULTIPLY; }
"/="			{ count (); return TOKEN_ASSIGN_DIVIDE; }
"%="			{ count (); return TOKEN_ASSIGN_REMAINDER; }
"+="			{ count (); return TOKEN_ASSIGN_ADD; }
"-="			{ count (); return TOKEN_ASSIGN_MINUS; }
"<<="			{ count (); return TOKEN_ASSIGN_SHIFT_LEFT; }
">>="			{ count (); return TOKEN_ASSIGN_SHIFT_RIGHT; }
">>>="			{ count (); return TOKEN_ASSIGN_SHIFT_RIGHT_ZERO; }
"&="			{ count (); return TOKEN_ASSIGN_BITWISE_AND; }
"^="			{ count (); return TOKEN_ASSIGN_BITWISE_XOR; }
"|="			{ count (); return TOKEN_ASSIGN_BITWISE_OR; }

"break"			{ count (); return TOKEN_BREAK; }
"case"			{ count (); return TOKEN_CASE; }
"catch"			{ count (); return TOKEN_CATCH; }
"continue"		{ count (); return TOKEN_CONTINUE; }
"default"		{ count (); return TOKEN_DEFAULT; }
"delete"		{ count (); return TOKEN_DELETE; }
"do"			{ count (); return TOKEN_DO; }
"else"			{ count (); return TOKEN_ELSE; }
"finally"		{ count (); return TOKEN_FINALLY; }
"for"			{ count (); return TOKEN_FOR; }
"function"		{ count (); return TOKEN_FUNCTION; }
"if"			{ count (); return TOKEN_IF; }
"in"			{ count (); return TOKEN_IN; }
"instanceof"		{ count (); return TOKEN_INSTANCEOF; }
"new"			{ count (); return TOKEN_NEW; }
"return"		{ count (); return TOKEN_RETURN; }
"switch"		{ count (); return TOKEN_SWITCH; }
"this"			{ count (); return TOKEN_THIS; }
"throw"			{ count (); return TOKEN_THROW; }
"try"			{ count (); return TOKEN_TRY; }
"typeof"		{ count (); return TOKEN_TYPEOF; }
"var"			{ count (); return TOKEN_VAR; }
"void"			{ count (); return TOKEN_VOID; }
"while"			{ count (); return TOKEN_WHILE; }
"with"			{ count (); return TOKEN_WITH; }

"abstract"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"boolean"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"byte"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"char"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"class"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"const"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"debugger"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"double"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"enum"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"export"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"extends"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"final"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"float"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"goto"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"implements"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"import"		{ count (); return TOKEN_RESERVED_KEYWORD; }
	/*"int"			{ count (); return TOKEN_RESERVED_KEYWORD; }*/
"interface"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"long"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"native"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"package"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"private"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"protected"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"public"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"short"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"static"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"super"			{ count (); return TOKEN_RESERVED_KEYWORD; }
"synchronized"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"throws"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"transient"		{ count (); return TOKEN_RESERVED_KEYWORD; }
"volatile"		{ count (); return TOKEN_RESERVED_KEYWORD; }

"undefined"		{ count (); return TOKEN_UNDEFINED; }

"null"			{ count (); return TOKEN_NULL; }
"true"			{
			  count ();
			  value->type = VALUE_TYPE_BOOLEAN;
			  value->value.v_boolean = 1;
			  return TOKEN_BOOLEAN;
			}
"false"			{
			  count ();
			  value->type = VALUE_TYPE_BOOLEAN;
			  value->value.v_boolean = 0;
			  return TOKEN_BOOLEAN;
			}

0[xX][0-9a-fA-F]+	{
			  count ();
			  value->type = VALUE_TYPE_NUMBER;
			  value->value.v_number =
			    g_ascii_strtoull (yytext, NULL, 16);
			  return TOKEN_NUMBER;
			}

([1-9][0-9]*|0)(\.[0-9]*)?([eE][+-]?[0-9]+)? {
			  count ();
			  value->type = VALUE_TYPE_NUMBER;
			  value->value.v_number = g_ascii_strtod (yytext, NULL);
			  return TOKEN_NUMBER;
			}

\.[0-9]+([eE][+-]?[0-9]+)? {
			  count ();
			  value->type = VALUE_TYPE_NUMBER;
			  value->value.v_number = g_ascii_strtod (yytext, NULL);
			  return TOKEN_NUMBER;
			}

\"			{
			  count ();
			  string = g_string_new ("");
			  BEGIN(str);
			}

<str>{
  \"			{
			  count ();
			  BEGIN(INITIAL);
			  value->type = VALUE_TYPE_STRING;
			  value->value.v_string = g_string_free (string, FALSE);
			  return TOKEN_STRING;
			}
  \n			{
			  count ();
			  BEGIN(INITIAL);
			  g_string_free (string, TRUE);
			  value->type = VALUE_TYPE_ERROR;
			  value->value.v_error = g_strdup (
			      "Unterminated string constant");
			  return TOKEN_ERROR;
			}
  \\[0-7]{1,3}		{
			  guint64 result;
			  count ();
			  result = g_ascii_strtoull (yytext + 1, NULL, 8);
			  if (result > 0xff || result == 0) {
			    value->type = VALUE_TYPE_ERROR;
			    value->value.v_error = g_strdup_printf (
				"Invalid escape sequence %s", yytext);
			    return TOKEN_ERROR;
			  } else {
			    g_string_append_c (string, result);
			  }
			}
  \\[0-9]+		{
			  count ();
			  value->type = VALUE_TYPE_ERROR;
			  value->value.v_error = g_strdup_printf (
			      "Invalid escape sequence %s", yytext);
			  return TOKEN_ERROR;
			}
  \\x[0-9a-fA-F]{2}	{
			  guint64 result;
			  count ();
			  result = g_ascii_strtoull (yytext + 2, NULL, 16);
			  if (result == 0) {
			    value->type = VALUE_TYPE_ERROR;
			    value->value.v_error = g_strdup_printf (
				"Invalid escape sequence %s", yytext);
			    return TOKEN_ERROR;
			  } else {
			    g_string_append_c (string, result);
			  }
			}
  \\b			{ count (); g_string_append_c (string, '\b'); }
  \\f			{ count (); g_string_append_c (string, '\f'); }
  \\n			{ count (); g_string_append_c (string, '\n'); }
  \\r			{ count (); g_string_append_c (string, '\r'); }
  \\t			{ count (); g_string_append_c (string, '\t'); }
  \\v			{ count (); g_string_append_c (string, '\v'); }
  \\.			{ count (); g_string_append_c (string, yytext[1]); }
  [^\\\n\"]+		{
			  char *p;
			  count ();
			  for (p = yytext; *p != '\0'; p++) {
			    g_string_append_c (string, *p);
			  }
			}
}

{identifier_start}({identifier_part})* {
			  count ();
			  value->type = VALUE_TYPE_IDENTIFIER;
			  value->value.v_identifier = g_strdup (yytext);
			  return TOKEN_IDENTIFIER;
			}

.			{
			  count ();
			  value->type = VALUE_TYPE_ERROR;
			  value->value.v_error = g_strdup_printf (
			      "Unknown character '%c'", yytext[0]);
			  return TOKEN_ERROR;
			}

%%

static void new_line (void)
{
  lex_line_number++;
}

static void count (void)
{
  lex_position += yyleng;
}
