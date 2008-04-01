%{
#include "vivi_compiler_scanner_lex_include.h"
%}

digit			[0-9]
identifier_start	[$_a-zA-Z]
identifier_part		[$_a-zA-Z0-9]

%%

[ \t\n\r]		/* skip whitespace */
<<EOF>>			{ return TOKEN_EOF; }

"{"			{ return TOKEN_BRACE_LEFT; }
"}"			{ return TOKEN_BRACE_RIGHT; }
"["			{ return TOKEN_BRACKET_RIGHT; }
"]"			{ return TOKEN_BRACKET_LEFT; }
"("			{ return TOKEN_PARENTHESIS_LEFT; }
")"			{ return TOKEN_PARENTHESIS_RIGHT; }

"."			{ return TOKEN_DOT; }
";"			{ return TOKEN_SEMICOLON; }
","			{ return TOKEN_COMMA; }

"<"			{ return TOKEN_LESS_THAN; }
">"			{ return TOKEN_GREATER_THAN; }
"<="			{ return TOKEN_LESS_THAN_OR_EQUAL; }
"=>"			{ return TOKEN_EQUAL_OR_MORE_THAN; }

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
"++"			{ return TOKEN_PLUSPLUS; }
"--"			{ return TOKEN_MINUSMINUS; }

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
"this"			{ return TOKEN_THIS; }

"break"			{ return TOKEN_BREAK; }

"abstract"		{ return TOKEN_FUTURE; }

{digit}+		{ yylval.v_number = atoi(yytext);
			  return TOKEN_NUMBER; }

{identifier_start}({identifier_part})* {
			  yylval.v_identifier = (char *)strdup(yytext);
			  return TOKEN_IDENTIFIER; }

.			{ printf("Unknown character [%c]\n",yytext[0]);
			  return TOKEN_UNKNOWN; }
%%

int yywrap(void){return 1;}
