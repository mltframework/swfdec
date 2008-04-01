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
"with"			{ return TOKEN_WITH; }

"abstract"		{ return TOKEN_FUTURE; }
"boolean"		{ return TOKEN_FUTURE; }
"byte"			{ return TOKEN_FUTURE; }
"char"			{ return TOKEN_FUTURE; }
"class"			{ return TOKEN_FUTURE; }
"const"			{ return TOKEN_FUTURE; }
"debugger"		{ return TOKEN_FUTURE; }
"double"		{ return TOKEN_FUTURE; }
"enum"			{ return TOKEN_FUTURE; }
"export"		{ return TOKEN_FUTURE; }
"extends"		{ return TOKEN_FUTURE; }
"final"			{ return TOKEN_FUTURE; }
"float"			{ return TOKEN_FUTURE; }
"goto"			{ return TOKEN_FUTURE; }
"implements"		{ return TOKEN_FUTURE; }
"import"		{ return TOKEN_FUTURE; }
"int"			{ return TOKEN_FUTURE; }
"interface"		{ return TOKEN_FUTURE; }
"long"			{ return TOKEN_FUTURE; }
"native"		{ return TOKEN_FUTURE; }
"package"		{ return TOKEN_FUTURE; }
"private"		{ return TOKEN_FUTURE; }
"protected"		{ return TOKEN_FUTURE; }
"public"		{ return TOKEN_FUTURE; }
"short"			{ return TOKEN_FUTURE; }
"static"		{ return TOKEN_FUTURE; }
"super"			{ return TOKEN_FUTURE; }
"synchronized"		{ return TOKEN_FUTURE; }
"throws"		{ return TOKEN_FUTURE; }
"transient"		{ return TOKEN_FUTURE; }
"volatile"		{ return TOKEN_FUTURE; }

{digit}+		{ yylval.v_number = atoi(yytext);
			  return TOKEN_NUMBER; }

{identifier_start}({identifier_part})* {
			  yylval.v_identifier = (char *)strdup(yytext);
			  return TOKEN_IDENTIFIER; }

.			{ printf("Unknown character [%c]\n",yytext[0]);
			  return TOKEN_UNKNOWN; }
%%

int yywrap(void){return 1;}
