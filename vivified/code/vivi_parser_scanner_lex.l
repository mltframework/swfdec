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

#define YY_DECL int vivi_parser_scanner_lex (void *yyscanner, ViviParserValue *value)
extern YY_DECL;

#define YY_EXTRA_TYPE ViviParserScanner *
}

%{
static GString *string = NULL;
#define ERROR(...) vivi_parser_scanner_error (yyextra, yylineno, yycolumn, __VA_ARGS__)

#define YY_USER_ACTION \
  value->line_number = yylineno; \
  value->column = yycolumn;
%}

%option			header-file="vivi_parser_scanner_lex.h"
%option			never-interactive
%option			noyywrap
%option			nostdinit
%option			outfile="vivi_parser_scanner_lex.c"
%option			prefix="vivi_parser_scanner_"
%option			reentrant
%option			warn
%option			yylineno

identifier_start	[$_a-zA-Z]
identifier_part		[$_a-zA-Z0-9]

%x			str
%x			comment

%%

"/*"			{ BEGIN(comment); }
<comment>{
  "*/"			{ BEGIN(INITIAL); }
  .			{ /* skip */ }
  <<EOF>>		{
			  BEGIN(INITIAL);
			  ERROR ("Unterminated comment");
			}
}

"//"[^\r\n]*		{ /* skip */ }

<<EOF>>			{ return TOKEN_NONE; }

[ \t]			{ /* skip */ }

\r\n			{ value->line_terminator = TRUE; }
[\r\n]			{ value->line_terminator = TRUE; }

"{"			{ return TOKEN_BRACE_LEFT; }
"}"			{ return TOKEN_BRACE_RIGHT; }
"["			{ return TOKEN_BRACKET_LEFT; }
"]"			{ return TOKEN_BRACKET_RIGHT; }
"("			{ return TOKEN_PARENTHESIS_LEFT; }
")"			{ return TOKEN_PARENTHESIS_RIGHT; }

"."			{ return TOKEN_DOT; }
";"			{ return TOKEN_SEMICOLON; }
","			{ return TOKEN_COMMA; }

"<"			{ return TOKEN_LESS_THAN; }
">"			{ return TOKEN_GREATER_THAN; }
"<="			{ return TOKEN_LESS_THAN_OR_EQUAL; }
">="			{ return TOKEN_EQUAL_OR_GREATER_THAN; }

"=="			{ return TOKEN_EQUAL; }
"!="			{ return TOKEN_NOT_EQUAL; }
"==="			{ return TOKEN_STRICT_EQUAL; }
"!=="			{ return TOKEN_NOT_STRICT_EQUAL; }

"+"			{ return TOKEN_PLUS; }
"-"			{ return TOKEN_MINUS; }
"*"			{ return TOKEN_MULTIPLY; }
"/"			{ return TOKEN_DIVIDE; }
"%"			{ return TOKEN_REMAINDER; }

"<<"			{ return TOKEN_SHIFT_LEFT; }
">>"			{ return TOKEN_SHIFT_RIGHT; }
">>>"			{ return TOKEN_SHIFT_RIGHT_UNSIGNED; }

"&"			{ return TOKEN_BITWISE_AND; }
"|"			{ return TOKEN_BITWISE_OR; }
"^"			{ return TOKEN_BITWISE_XOR; }

"!"			{ return TOKEN_LOGICAL_NOT; }
"~"			{ return TOKEN_BITWISE_NOT; }
"++"			{ return TOKEN_INCREASE; }
"--"			{ return TOKEN_DESCREASE; }

"?"			{ return TOKEN_QUESTION_MARK; }
":"			{ return TOKEN_COLON; }

"&&"			{ return TOKEN_LOGICAL_AND; }
"||"			{ return TOKEN_LOGICAL_OR; }

"="			{ return TOKEN_ASSIGN; }
"*="			{ return TOKEN_ASSIGN_MULTIPLY; }
"/="			{ return TOKEN_ASSIGN_DIVIDE; }
"%="			{ return TOKEN_ASSIGN_REMAINDER; }
"+="			{ return TOKEN_ASSIGN_ADD; }
"-="			{ return TOKEN_ASSIGN_MINUS; }
"<<="			{ return TOKEN_ASSIGN_SHIFT_LEFT; }
">>="			{ return TOKEN_ASSIGN_SHIFT_RIGHT; }
">>>="			{ return TOKEN_ASSIGN_SHIFT_RIGHT_ZERO; }
"&="			{ return TOKEN_ASSIGN_BITWISE_AND; }
"^="			{ return TOKEN_ASSIGN_BITWISE_XOR; }
"|="			{ return TOKEN_ASSIGN_BITWISE_OR; }

"break"			{ return TOKEN_BREAK; }
"case"			{ return TOKEN_CASE; }
"catch"			{ return TOKEN_CATCH; }
"continue"		{ return TOKEN_CONTINUE; }
"default"		{ return TOKEN_DEFAULT; }
"delete"		{ return TOKEN_DELETE; }
"do"			{ return TOKEN_DO; }
"else"			{ return TOKEN_ELSE; }
"finally"		{ return TOKEN_FINALLY; }
"for"			{ return TOKEN_FOR; }
"function"		{ return TOKEN_FUNCTION; }
"if"			{ return TOKEN_IF; }
"in"			{ return TOKEN_IN; }
"instanceof"		{ return TOKEN_INSTANCEOF; }
"new"			{ return TOKEN_NEW; }
"return"		{ return TOKEN_RETURN; }
"switch"		{ return TOKEN_SWITCH; }
"this"			{ return TOKEN_THIS; }
"throw"			{ return TOKEN_THROW; }
"try"			{ return TOKEN_TRY; }
"typeof"		{ return TOKEN_TYPEOF; }
"var"			{ return TOKEN_VAR; }
"void"			{ return TOKEN_VOID; }
"while"			{ return TOKEN_WHILE; }
"with"			{ return TOKEN_WITH; }

"abstract"		{ return TOKEN_RESERVED_KEYWORD; }
"boolean"		{ return TOKEN_RESERVED_KEYWORD; }
"byte"			{ return TOKEN_RESERVED_KEYWORD; }
"char"			{ return TOKEN_RESERVED_KEYWORD; }
"class"			{ return TOKEN_RESERVED_KEYWORD; }
"const"			{ return TOKEN_RESERVED_KEYWORD; }
"debugger"		{ return TOKEN_RESERVED_KEYWORD; }
"double"		{ return TOKEN_RESERVED_KEYWORD; }
"enum"			{ return TOKEN_RESERVED_KEYWORD; }
"export"		{ return TOKEN_RESERVED_KEYWORD; }
"extends"		{ return TOKEN_RESERVED_KEYWORD; }
"final"			{ return TOKEN_RESERVED_KEYWORD; }
"float"			{ return TOKEN_RESERVED_KEYWORD; }
"goto"			{ return TOKEN_RESERVED_KEYWORD; }
"implements"		{ return TOKEN_RESERVED_KEYWORD; }
"import"		{ return TOKEN_RESERVED_KEYWORD; }
	/*"int"			{ return TOKEN_RESERVED_KEYWORD; }*/
"interface"		{ return TOKEN_RESERVED_KEYWORD; }
"long"			{ return TOKEN_RESERVED_KEYWORD; }
"native"		{ return TOKEN_RESERVED_KEYWORD; }
"package"		{ return TOKEN_RESERVED_KEYWORD; }
"private"		{ return TOKEN_RESERVED_KEYWORD; }
"protected"		{ return TOKEN_RESERVED_KEYWORD; }
"public"		{ return TOKEN_RESERVED_KEYWORD; }
"short"			{ return TOKEN_RESERVED_KEYWORD; }
"static"		{ return TOKEN_RESERVED_KEYWORD; }
"super"			{ return TOKEN_RESERVED_KEYWORD; }
"synchronized"		{ return TOKEN_RESERVED_KEYWORD; }
"throws"		{ return TOKEN_RESERVED_KEYWORD; }
"transient"		{ return TOKEN_RESERVED_KEYWORD; }
"volatile"		{ return TOKEN_RESERVED_KEYWORD; }

"undefined"		{ return TOKEN_UNDEFINED; }

"null"			{ return TOKEN_NULL; }
"true"			{
			  value->value.v_boolean = 1;
			  return TOKEN_BOOLEAN;
			}
"false"			{
			  value->value.v_boolean = 0;
			  return TOKEN_BOOLEAN;
			}

0[xX][0-9a-fA-F]+	{
			  value->value.v_number =
			    g_ascii_strtoull (yytext, NULL, 16);
			  return TOKEN_NUMBER;
			}

([1-9][0-9]*|0)(\.[0-9]*)?([eE][+-]?[0-9]+)? {
			  value->value.v_number = g_ascii_strtod (yytext, NULL);
			  return TOKEN_NUMBER;
			}

\.[0-9]+([eE][+-]?[0-9]+)? {
			  value->value.v_number = g_ascii_strtod (yytext, NULL);
			  return TOKEN_NUMBER;
			}

\"			{
			  string = g_string_new ("");
			  BEGIN(str);
			}

<str>{
  \"			{
			  BEGIN(INITIAL);
			  value->value.v_string = g_string_free (string, FALSE);
			  return TOKEN_STRING;
			}
  \n			{
			  BEGIN(INITIAL);
			  g_string_free (string, TRUE);
			  ERROR ("Unterminated string constant");
			}
  \\[0-7]{1,3}		{
			  guint64 result;
			  result = g_ascii_strtoull (yytext + 1, NULL, 8);
			  if (result > 0xff || result == 0) {
			    ERROR ("Invalid escape sequence %s", yytext);
			  } else {
			    g_string_append_c (string, result);
			  }
			}
  \\[0-9]+		{ ERROR ("Invalid escape sequence %s", yytext); }
  \\x[0-9a-fA-F]{2}	{
			  guint64 result;
			  result = g_ascii_strtoull (yytext + 2, NULL, 16);
			  if (result == 0) {
			    ERROR ("Invalid escape sequence %s", yytext);
			  } else {
			    g_string_append_c (string, result);
			  }
			}
  \\b			{ g_string_append_c (string, '\b'); }
  \\f			{ g_string_append_c (string, '\f'); }
  \\n			{ g_string_append_c (string, '\n'); }
  \\r			{ g_string_append_c (string, '\r'); }
  \\t			{ g_string_append_c (string, '\t'); }
  \\v			{ g_string_append_c (string, '\v'); }
  \\.			{ g_string_append_c (string, yytext[1]); }
  [^\\\n\"]+		{
			  char *p;
			  for (p = yytext; *p != '\0'; p++) {
			    g_string_append_c (string, *p);
			  }
			}
}

{identifier_start}({identifier_part})* {
			  value->value.v_identifier = g_strdup (yytext);
			  return TOKEN_IDENTIFIER;
			}

.			{ ERROR ("Unknown character '%c'", yytext[0]); }

%%

