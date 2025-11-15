#ifndef TOYC_PARSER_H
#define TOYC_PARSER_H

#include "token.h"
#include "lexer.h"
#include "symbol.h"
#include <vector>
#include <string>
#include <set>

class Parser {
public:
    Parser(const std::vector<Token> &tokens);
    void parse(); // parse whole compunit
    bool is_accept() const;
    const std::vector<int>& get_errors() const;

private:
    const std::vector<Token> tokens;
    size_t pos;
    Token cur;

    std::vector<int> errors; // collected error line numbers in occurrence order
    std::set<std::string> functions_declared; // declared function names (before their use)
    std::string current_function; // name of function under parsing (for recursion allow)
    std::unordered_map<std::string, FuncInfo> func_table;

    SymTable vartable;

    // helpers
    void advance();
    bool match(TokenType t);
    bool check(TokenType t) const;
    void add_error_line(int ln);
    void sync_to_stmt();

    // grammar
    void CompUnit();
    bool FuncDef();
    bool Param();
    bool Block();
    bool Stmt();
    bool Expr(); // returns true if parsed an expression
    bool LOrExpr();
    bool LAndExpr();
    bool RelExpr();
    bool AddExpr();
    bool MulExpr();
    bool UnaryExpr();
    bool PrimaryExpr();

    // semantic helpers
    bool expect_and_consume(TokenType t);
};

#endif
