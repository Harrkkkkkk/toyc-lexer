%code requires {
#include <string>
#include <vector>
#include <memory>
#include "parser/ast.h"
}

%{
#include <iostream>
#include <memory>
#include "parser/ast.h"

extern int yylex();
extern int yyparse();
extern int yylineno;
void yyerror(const char* s);

std::shared_ptr<CompUnit> root;
%}

%union {
    int num;
    std::string* str;
    ASTNode* node;
    Expr* expr;
    Stmt* stmt;
    BlockStmt* block;
    FunctionDef* func;
    CompUnit* unit;
    std::vector<Param>* params;
    std::vector<std::shared_ptr<Stmt>>* stmts;
    std::vector<std::shared_ptr<Expr>>* args;
    std::vector<std::shared_ptr<FunctionDef>>* funcs;
}

%token <str> IDENTIFIER
%token <num> NUMBER
%token INT VOID IF ELSE WHILE BREAK CONTINUE RETURN CONST
%token PLUS MINUS MULTIPLY DIVIDE MODULO
%token ASSIGN EQ NEQ LT GT LE GE AND OR NOT
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA

%type <unit> comp_unit
%type <funcs> func_list
%type <func> func_def
%type <str> type
%type <params> params param_list
%type <block> block
%type <stmts> stmt_list
%type <stmt> stmt var_decl assign_stmt if_stmt while_stmt return_stmt expr_stmt
%type <expr> expr lor_expr land_expr eq_expr rel_expr add_expr mul_expr unary_expr primary_expr
%type <args> args arg_list

%start comp_unit

%%

comp_unit: func_list {
    $$ = new CompUnit(*$1, yylineno);
    root = std::shared_ptr<CompUnit>($$);
    delete $1;
}

func_list: func_list func_def {
    $$ = $1;
    $$->push_back(std::shared_ptr<FunctionDef>($2));
}
| func_def {
    $$ = new std::vector<std::shared_ptr<FunctionDef>>();
    $$->push_back(std::shared_ptr<FunctionDef>($1));
}

func_def: type IDENTIFIER LPAREN params RPAREN block {
    $$ = new FunctionDef(*$1, *$2, *$4, std::shared_ptr<BlockStmt>($6), yylineno);
    delete $1; delete $2; delete $4;
}

type: INT { $$ = new std::string("int"); }
| VOID { $$ = new std::string("void"); }

params: param_list { $$ = $1; }
| /* empty */ { $$ = new std::vector<Param>(); }

param_list: param_list COMMA INT IDENTIFIER {
    $$ = $1;
    $$->push_back(Param(*$4, yylineno));
    delete $4;
}
| INT IDENTIFIER {
    $$ = new std::vector<Param>();
    $$->push_back(Param(*$2, yylineno));
    delete $2;
}

block: LBRACE stmt_list RBRACE {
    $$ = new BlockStmt(*$2, yylineno);
    delete $2;
}

stmt_list: stmt_list stmt {
    $$ = $1;
    if ($2) $$->push_back(std::shared_ptr<Stmt>($2));
}
| /* empty */ {
    $$ = new std::vector<std::shared_ptr<Stmt>>();
}

stmt: var_decl { $$ = $1; }
| assign_stmt { $$ = $1; }
| if_stmt { $$ = $1; }
| while_stmt { $$ = $1; }
| return_stmt { $$ = $1; }
| BREAK SEMICOLON { $$ = new BreakStmt(yylineno); }
| CONTINUE SEMICOLON { $$ = new ContinueStmt(yylineno); }
| expr_stmt { $$ = $1; }
| block { $$ = $1; }
| SEMICOLON { $$ = nullptr; } // Empty statement

var_decl: INT IDENTIFIER ASSIGN expr SEMICOLON {
    $$ = new VarDeclStmt(*$2, std::shared_ptr<Expr>($4), yylineno);
    delete $2;
}
| CONST INT IDENTIFIER ASSIGN expr SEMICOLON {
    $$ = new VarDeclStmt(*$3, std::shared_ptr<Expr>($5), yylineno);
    delete $3;
}
| INT IDENTIFIER SEMICOLON {
    $$ = new VarDeclStmt(*$2, nullptr, yylineno);
    delete $2;
}

assign_stmt: IDENTIFIER ASSIGN expr SEMICOLON {
    $$ = new AssignStmt(*$1, std::shared_ptr<Expr>($3), yylineno);
    delete $1;
}

if_stmt: IF LPAREN expr RPAREN stmt ELSE stmt {
    $$ = new IfStmt(std::shared_ptr<Expr>($3), std::shared_ptr<Stmt>($5), std::shared_ptr<Stmt>($7), yylineno);
}
| IF LPAREN expr RPAREN stmt {
    $$ = new IfStmt(std::shared_ptr<Expr>($3), std::shared_ptr<Stmt>($5), nullptr, yylineno);
}

while_stmt: WHILE LPAREN expr RPAREN stmt {
    $$ = new WhileStmt(std::shared_ptr<Expr>($3), std::shared_ptr<Stmt>($5), yylineno);
}

return_stmt: RETURN expr SEMICOLON {
    $$ = new ReturnStmt(std::shared_ptr<Expr>($2), yylineno);
}
| RETURN SEMICOLON {
    $$ = new ReturnStmt(nullptr, yylineno);
}

expr_stmt: expr SEMICOLON {
    $$ = new ExprStmt(std::shared_ptr<Expr>($1), yylineno);
}

expr: lor_expr { $$ = $1; }

lor_expr: lor_expr OR land_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "||", std::shared_ptr<Expr>($3), yylineno);
}
| land_expr { $$ = $1; }

land_expr: land_expr AND eq_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "&&", std::shared_ptr<Expr>($3), yylineno);
}
| eq_expr { $$ = $1; }

eq_expr: eq_expr EQ rel_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "==", std::shared_ptr<Expr>($3), yylineno);
}
| eq_expr NEQ rel_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "!=", std::shared_ptr<Expr>($3), yylineno);
}
| rel_expr { $$ = $1; }

rel_expr: rel_expr LT add_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "<", std::shared_ptr<Expr>($3), yylineno);
}
| rel_expr GT add_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), ">", std::shared_ptr<Expr>($3), yylineno);
}
| rel_expr LE add_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "<=", std::shared_ptr<Expr>($3), yylineno);
}
| rel_expr GE add_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), ">=", std::shared_ptr<Expr>($3), yylineno);
}
| add_expr { $$ = $1; }

add_expr: add_expr PLUS mul_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "+", std::shared_ptr<Expr>($3), yylineno);
}
| add_expr MINUS mul_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "-", std::shared_ptr<Expr>($3), yylineno);
}
| mul_expr { $$ = $1; }

mul_expr: mul_expr MULTIPLY unary_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "*", std::shared_ptr<Expr>($3), yylineno);
}
| mul_expr DIVIDE unary_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "/", std::shared_ptr<Expr>($3), yylineno);
}
| mul_expr MODULO unary_expr {
    $$ = new BinaryExpr(std::shared_ptr<Expr>($1), "%", std::shared_ptr<Expr>($3), yylineno);
}
| unary_expr { $$ = $1; }

unary_expr: PLUS unary_expr {
    $$ = new UnaryExpr("+", std::shared_ptr<Expr>($2), yylineno);
}
| MINUS unary_expr {
    $$ = new UnaryExpr("-", std::shared_ptr<Expr>($2), yylineno);
}
| NOT unary_expr {
    $$ = new UnaryExpr("!", std::shared_ptr<Expr>($2), yylineno);
}
| primary_expr { $$ = $1; }

primary_expr: LPAREN expr RPAREN { $$ = $2; }
| NUMBER { $$ = new NumberExpr($1, yylineno); }
| IDENTIFIER {
    $$ = new VariableExpr(*$1, yylineno);
    delete $1;
}
| IDENTIFIER LPAREN args RPAREN {
    $$ = new CallExpr(*$1, *$3, yylineno);
    delete $1; delete $3;
}

args: arg_list { $$ = $1; }
| /* empty */ { $$ = new std::vector<std::shared_ptr<Expr>>(); }

arg_list: arg_list COMMA expr {
    $$ = $1;
    $$->push_back(std::shared_ptr<Expr>($3));
}
| expr {
    $$ = new std::vector<std::shared_ptr<Expr>>();
    $$->push_back(std::shared_ptr<Expr>($1));
}

%%

void yyerror(const char* s) {
    std::cerr << "Error: " << s << " at line " << yylineno << std::endl;
}