%{
#include "parser.hpp"
#include <string>
#include <iostream>
%}

%option noyywrap
%option yylineno

%x COMMENT

%%
[ \t\n\r]+  ; // Skip whitespace
"//".*      ; // Skip single-line comments

"/*"        { BEGIN(COMMENT); }
<COMMENT>"*/" { BEGIN(INITIAL); }
<COMMENT>(.|\n) ; // Skip content inside multi-line comments

"int"       { return INT; }
"const"     { return CONST; }
"void"      { return VOID; }
"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"break"     { return BREAK; }
"continue"  { return CONTINUE; }
"return"    { return RETURN; }

[0-9]+      { yylval.num = std::stoi(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.str = new std::string(yytext); return IDENTIFIER; }

"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MULTIPLY; }
"/"         { return DIVIDE; }
"%"         { return MODULO; }
"="         { return ASSIGN; }
"=="        { return EQ; }
"!="        { return NEQ; }
"<"         { return LT; }
">"         { return GT; }
"<="        { return LE; }
">="        { return GE; }
"&&"        { return AND; }
"||"        { return OR; }
"!"         { return NOT; }
"("         { return LPAREN; }
")"         { return RPAREN; }
"{"         { return LBRACE; }
"}"         { return RBRACE; }
";"         { return SEMICOLON; }
","         { return COMMA; }

.           { std::cerr << "Unknown character: " << yytext << std::endl; }
%%