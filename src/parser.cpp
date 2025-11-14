#include "parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token> &toks): tokens(toks), pos(0) {
    cur = tokens[pos];
}

void Parser::advance() {
    if (pos + 1 < tokens.size()) ++pos;
    cur = tokens[pos];
}

bool Parser::check(TokenType t) const { return cur.type == t; }
bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

void Parser::add_error_line(int ln) {
    // add in order, avoid duplicates for same immediate spot
    if (errors.empty() || errors.back() != ln) errors.push_back(ln);
}

void Parser::sync_to_stmt() {
    // skip tokens until a statement boundary to continue parsing
    static std::set<TokenType> sync = {
        T_SEMI, T_LBRACE, T_RBRACE, T_KW_INT, T_KW_VOID, T_KW_IF, T_KW_WHILE, T_KW_RETURN, T_KW_BREAK, T_KW_CONTINUE
    };
    while (!check(T_EOF) && !sync.count(cur.type)) advance();
    // if semicolon, consume it to avoid loop
    if (check(T_SEMI)) advance();
}

void Parser::parse() {
    CompUnit();
    // semantic: check main presence
    if (!functions_declared.count("main")) {
        // approximate: find first token line if none else 1
        int ln = tokens.empty() ? 1 : tokens.front().line;
        add_error_line(ln);
    } else {
        // ensure main signature: int main()
        auto it = func_table.find("main");
        if (it == func_table.end()) {
            add_error_line(tokens.front().line);
        } else {
            if (!it->second.returns_int || it->second.params.size() != 0) {
                add_error_line(it->second.declared_line);
            }
        }
    }
}

bool Parser::is_accept() const {
    return errors.empty();
}

const std::vector<int>& Parser::get_errors() const { return errors; }

// CompUnit → FuncDef+
void Parser::CompUnit() {
    while (!check(T_EOF)) {
        if (!FuncDef()) {
            // error recovery: skip to next top-level (int/void) or EOF
            add_error_line(cur.line);
            while (!check(T_EOF) && cur.type != T_KW_INT && cur.type != T_KW_VOID) advance();
        }
    }
}

/*
FuncDef → (“int” | “void”) ID “(” (Param (“,” Param)*)? “)” Block
*/
bool Parser::FuncDef() {
    if (!(check(T_KW_INT) || check(T_KW_VOID))) return false;
    bool returns_int = check(T_KW_INT);
    int decl_line = cur.line;
    advance(); // int/void
    if (!check(T_ID)) {
        add_error_line(cur.line);
        sync_to_stmt();
        return false;
    }
    std::string fname = cur.lexeme;
    if (functions_declared.count(fname)) {
        // function name duplicate
        add_error_line(cur.line);
    }
    // register function as declared now (so calls later see it)
    functions_declared.insert(fname);
    FuncInfo finfo;
    finfo.name = fname; finfo.returns_int = returns_int; finfo.declared_line = decl_line;
    advance(); // ID
    if (!match(T_LPAREN)) {
        add_error_line(cur.line);
        sync_to_stmt();
        return false;
    }
    // params
    finfo.params.clear();
    if (!check(T_RPAREN)) {
        // at least one param
        while (true) {
            if (!Param()) {
                add_error_line(cur.line);
                sync_to_stmt();
                break;
            } else {
                // param was consumed: last seen ID token was param name
                // We don't have direct param name here; for simplicity we won't store names reliably
                // but store placeholder
                finfo.params.push_back("p");
            }
            if (match(T_COMMA)) continue;
            else break;
        }
    }
    if (!match(T_RPAREN)) {
        add_error_line(cur.line);
        // try to continue
    }
    // now block
    current_function = fname;
    func_table[fname] = finfo;
    // enter function scope
    vartable.push_scope();
    bool ok = Block();
    // leave function scope
    vartable.pop_scope();
    current_function.clear();
    return ok;
}

/*
Param → “int” ID
*/
bool Parser::Param() {
    if (!match(T_KW_INT)) return false;
    if (!check(T_ID)) { add_error_line(cur.line); return false; }
    // declare param as variable in current scope
    std::string pname = cur.lexeme;
    vartable.declare_var(pname, cur.line);
    advance();
    return true;
}

/*
Block → “{” Stmt* “}”
*/
bool Parser::Block() {
    if (!match(T_LBRACE)) { add_error_line(cur.line); return false; }
    // new scope
    vartable.push_scope();
    while (!check(T_RBRACE) && !check(T_EOF)) {
        if (!Stmt()) {
            // error recorded inside; attempt to sync and continue
            sync_to_stmt();
        }
    }
    if (!match(T_RBRACE)) {
        add_error_line(cur.line);
        vartable.pop_scope();
        return false;
    }
    vartable.pop_scope();
    return true;
}

/*
Stmt → Block | “;” | Expr “;” | ID “=” Expr “;”
     | “int” ID “=” Expr “;”
     | “if” “(” Expr “)” Stmt (“else” Stmt)?
     | “while” “(” Expr “)” Stmt
     | “break” “;” | “continue” “;” | “return” Expr “;”
*/
bool Parser::Stmt() {
    if (check(T_LBRACE)) {
        return Block();
    }
    if (match(T_SEMI)) return true; // empty

    if (check(T_KW_INT)) {
        // variable declaration with initialization required
        int ln = cur.line;
        advance();
        if (!check(T_ID)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        std::string vname = cur.lexeme;
        // declare variable in current scope
        bool okdecl = vartable.declare_var(vname, cur.line);
        advance();
        if (!match(T_ASSIGN)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        if (!Expr()) { add_error_line(cur.line); sync_to_stmt(); return false; }
        if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        return true;
    }

    if (check(T_KW_IF)) {
        advance();
        if (!match(T_LPAREN)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        if (!Expr()) { add_error_line(cur.line); sync_to_stmt(); }
        if (!match(T_RPAREN)) { add_error_line(cur.line); sync_to_stmt(); }
        if (!Stmt()) { add_error_line(cur.line); sync_to_stmt(); }
        if (check(T_KW_ELSE)) {
            advance();
            if (!Stmt()) { add_error_line(cur.line); sync_to_stmt(); }
        }
        return true;
    }

    if (check(T_KW_WHILE)) {
        advance();
        if (!match(T_LPAREN)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        if (!Expr()) { add_error_line(cur.line); sync_to_stmt(); }
        if (!match(T_RPAREN)) { add_error_line(cur.line); sync_to_stmt(); }
        if (!Stmt()) { add_error_line(cur.line); sync_to_stmt(); }
        return true;
    }

    if (check(T_KW_BREAK)) {
        advance();
        // check that we are inside a loop? We don't track loop nesting precisely; a conservative approach: allow but could flag
        if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); }
        return true;
    }
    if (check(T_KW_CONTINUE)) {
        advance();
        if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); }
        return true;
    }

    if (check(T_KW_RETURN)) {
        advance();
        // parse expr (could be empty only for void; but spec: void return must be without value)
        if (!Expr()) {
            // allow empty expression? no, grammar required return Expr ";"
            add_error_line(cur.line);
            sync_to_stmt();
            if (check(T_SEMI)) advance();
            return false;
        }
        if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); }
        return true;
    }

    // assignment or expression statement
    if (check(T_ID)) {
        // lookahead
        size_t save = pos;
        std::string name = cur.lexeme;
        advance();
        if (check(T_ASSIGN)) {
            advance();
            if (!Expr()) { add_error_line(cur.line); sync_to_stmt(); return false; }
            if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); return false; }
            // check var declared before
            if (!vartable.has_var(name)) add_error_line(tokens[save].line);
            return true;
        } else {
            // function call or variable expr: reset to saved pos and parse expr
            pos = save;
            cur = tokens[pos];
            if (!Expr()) { add_error_line(cur.line); sync_to_stmt(); return false; }
            if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); return false; }
            return true;
        }
    }

    // otherwise expression statement
    if (Expr()) {
        if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        return true;
    }

    // unknown start
    add_error_line(cur.line);
    sync_to_stmt();
    return false;
}

/*
Expressions implementation with precedence:
Expr → LOrExpr
LOrExpr → LAndExpr | LOrExpr "||" LAndExpr
LAndExpr → RelExpr | LAndExpr "&&" RelExpr
RelExpr → AddExpr (relop AddExpr)*
AddExpr → MulExpr ((+|-) MulExpr)*
MulExpr → UnaryExpr ((*|/|%) UnaryExpr)*
UnaryExpr → PrimaryExpr | (+|-|!) UnaryExpr
PrimaryExpr → ID | NUMBER | "(" Expr ")" | ID "(" (Expr ("," Expr)*)? ")"
*/
bool Parser::Expr() { return LOrExpr(); }
bool Parser::LOrExpr() {
    if (!LAndExpr()) return false;
    while (check(T_OROR)) { advance(); if (!LAndExpr()) { add_error_line(cur.line); return false; } }
    return true;
}
bool Parser::LAndExpr() {
    if (!RelExpr()) return false;
    while (check(T_ANDAND)) { advance(); if (!RelExpr()) { add_error_line(cur.line); return false; } }
    return true;
}
bool Parser::RelExpr() {
    if (!AddExpr()) return false;
    while (check(T_LT) || check(T_GT) || check(T_LE) || check(T_GE) || check(T_EQ) || check(T_NEQ)) {
        advance();
        if (!AddExpr()) { add_error_line(cur.line); return false; }
    }
    return true;
}
bool Parser::AddExpr() {
    if (!MulExpr()) return false;
    while (check(T_PLUS) || check(T_MINUS)) { advance(); if (!MulExpr()) { add_error_line(cur.line); return false; } }
    return true;
}
bool Parser::MulExpr() {
    if (!UnaryExpr()) return false;
    while (check(T_MUL) || check(T_DIV) || check(T_MOD)) { advance(); if (!UnaryExpr()) { add_error_line(cur.line); return false; } }
    return true;
}
bool Parser::UnaryExpr() {
    if (check(T_PLUS) || check(T_MINUS) || check(T_NOT)) { advance(); return UnaryExpr(); }
    return PrimaryExpr();
}
bool Parser::PrimaryExpr() {
    if (check(T_ID)) {
        std::string name = cur.lexeme;
        int ln = cur.line;
        advance();
        if (match(T_LPAREN)) {
            // function call
            // arguments optional
            if (!check(T_RPAREN)) {
                while (true) {
                    if (!Expr()) { add_error_line(cur.line); return false; }
                    if (match(T_COMMA)) continue;
                    else break;
                }
            }
            if (!match(T_RPAREN)) { add_error_line(cur.line); return false; }
            // semantic: function must be declared earlier or be current function (allow recursion)
            if (!(functions_declared.count(name) || name == current_function)) {
                add_error_line(ln);
            }
            return true;
        } else {
            // variable usage
            if (!vartable.has_var(name)) {
                add_error_line(ln);
            }
            return true;
        }
    } else if (check(T_NUMBER)) {
        advance(); return true;
    } else if (match(T_LPAREN)) {
        if (!Expr()) { add_error_line(cur.line); return false; }
        if (!match(T_RPAREN)) { add_error_line(cur.line); return false; }
        return true;
    }
    return false;
}
