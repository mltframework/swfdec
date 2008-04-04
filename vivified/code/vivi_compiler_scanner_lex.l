%{
#include "vivi_compiler_scanner_lex_include.h"
%}

%option			noyywrap

digit			[0-9]
identifier_start	[$_a-zA-Z]
identifier_part		[$_a-zA-Z0-9]
line_terminator		[\n\r]

/* r'([1-9][0-9]*|0)(\.[0-9]*)?([eE][+-][0-9]+)? */
/* '\.[0-9]+([eE][+-][0-9]+)?' */

%%

"//".*{line_terminator}	/* skip single line comments */
[ \t]			/* skip whitespace */
<<EOF>>			{ return TOKEN_EOF; }
[\n\r]			{ return TOKEN_LINE_TERMINATOR; }

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
"=>"			{ return TOKEN_EQUAL_OR_GREATER_THAN; }

"==",			{ return TOKEN_EQUAL; }
"!=",			{ return TOKEN_NOT_EQUAL; }
"===",			{ return TOKEN_STRICT_EQUAL; }
"!==",			{ return TOKEN_NOT_STRICT_EQUAL; }

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

"null"			{ return TOKEN_NULL; }
"true"			{ yylval.v_boolean = 1;
			  return TOKEN_BOOLEAN; }
"false"			{ yylval.v_boolean = 0;
			  return TOKEN_BOOLEAN; }

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
"int"			{ return TOKEN_RESERVED_KEYWORD; }
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

[-+]*{digit}+		{ yylval.v_number = atoi(yytext);
			  return TOKEN_NUMBER; }
"\"".*"\""		{ yylval.v_string = g_strndup (yytext + 1, yyleng - 2);
			  return TOKEN_STRING; }

{identifier_start}({identifier_part})* {
			  yylval.v_identifier = g_strdup (yytext);
			  return TOKEN_IDENTIFIER; }

.			{ printf("Unknown character [%c]\n",yytext[0]);
			  return TOKEN_UNKNOWN; }
%%
