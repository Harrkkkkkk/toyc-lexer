// parser.cpp
#include "parser.h"
#include <iostream>

// NOTE: This implementation assumes parser.h / token.h / lexer.cpp / symbol.h
// are the ones you provided. It keeps interfaces unchanged.

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
    // Add in order, avoid duplicates for immediate repeats
    if (errors.empty() || errors.back() != ln) errors.push_back(ln);
}

void Parser::sync_to_stmt() {
    // Skip tokens until a reasonable statement boundary
    static std::set<TokenType> sync = {
        T_SEMI, T_LBRACE, T_RBRACE, T_KW_INT, T_KW_VOID, T_KW_IF, T_KW_WHILE, T_KW_RETURN, T_KW_BREAK, T_KW_CONTINUE
    };
    while (!check(T_EOF) && !sync.count(cur.type)) advance();
    if (check(T_SEMI)) advance();
}

void Parser::parse() {
    CompUnit();

    // semantic: check main presence and signature
    if (!functions_declared.count("main")) {
        int ln = tokens.empty() ? 1 : tokens.front().line;
        add_error_line(ln);
    } else {
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
We parse parameters inline and store their names in finfo.params.
We DO NOT push a scope here; Block() will push scope and inject params into scope.
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
        // duplicate function name
        add_error_line(cur.line);
    }
    functions_declared.insert(fname);

    FuncInfo finfo;
    finfo.name = fname; finfo.returns_int = returns_int; finfo.declared_line = decl_line;
    finfo.params.clear();

    advance(); // ID

    if (!match(T_LPAREN)) {
        add_error_line(cur.line);
        sync_to_stmt();
        return false;
    }

    // parse parameter list inline: (Param (',' Param)*)?
    if (!check(T_RPAREN)) {
        while (true) {
            // Param -> "int" ID
            if (!match(T_KW_INT)) {
                add_error_line(cur.line);
                sync_to_stmt();
                break;
            }
            if (!check(T_ID)) {
                add_error_line(cur.line);
                sync_to_stmt();
                break;
            }
            // capture param name
            finfo.params.push_back(cur.lexeme);
            // DO NOT declare param now; it will be injected when entering Block()
            advance(); // consume ID

            if (match(T_COMMA)) {
                // continue parsing params
                continue;
            } else break;
        }
    }

    if (!match(T_RPAREN)) {
        add_error_line(cur.line);
        // attempt to continue
    }

    // register function info before parsing body (so calls can be checked)
    func_table[fname] = finfo;
    // mark declared
    functions_declared.insert(fname);

    // set current function for return checks and recursive calls
    current_function = fname;

    // Parse Block (Block will push scope and inject params)
    bool ok = Block();

    // clear current function
    current_function.clear();

    return ok;
}

/*
Param() - not used by FuncDef anymore, but implement in case other code calls it.
*/
bool Parser::Param() {
    if (!match(T_KW_INT)) return false;
    if (!check(T_ID)) { add_error_line(cur.line); return false; }
    std::string pname = cur.lexeme;
    // best-effort: declare onto current scope if any
    vartable.declare_var(pname, cur.line);
    advance();
    return true;
}

/*
Block → “{” Stmt* “}”
When entering Block we push scope and inject current function's params (if any)
*/
bool Parser::Block() {
    if (!match(T_LBRACE)) { add_error_line(cur.line); return false; }

    // new scope for block
    vartable.push_scope();

    // inject function parameters into this scope
    if (!current_function.empty()) {
        auto it = func_table.find(current_function);
        if (it != func_table.end()) {
            for (auto &pname : it->second.params) {
                // declare param in current scope with function decl line
                vartable.declare_var(pname, it->second.declared_line);
            }
        }
    }

    while (!check(T_RBRACE) && !check(T_EOF)) {
        if (!Stmt()) {
            // try to recover and continue
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
     | “break” “;” | “continue” “;” | “return” (Expr)? “;”
*/
bool Parser::Stmt() {
    if (check(T_LBRACE)) {
        return Block();
    }
    if (match(T_SEMI)) return true; // empty statement

    if (check(T_KW_INT)) {
        // variable declaration must have initialization
        advance(); // consume 'int'
        if (!check(T_ID)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        std::string vname = cur.lexeme;
        advance(); // consume ID
        if (!match(T_ASSIGN)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        if (!Expr()) { add_error_line(cur.line); sync_to_stmt(); return false; }
        if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        // declare in current scope
        vartable.declare_var(vname, cur.line);
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
        if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); }
        // (Optional) a semantic check for break in loop could be added
        return true;
    }
    if (check(T_KW_CONTINUE)) {
        advance();
        if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); }
        // (Optional) semantic check for continue in loop
        return true;
    }

    if (check(T_KW_RETURN)) {
        int ln = cur.line;
        advance(); // consume 'return'

        bool has_expr = false;
        size_t save_pos = pos;
        Token save_cur = cur;

        // allow both 'return;' and 'return expr;'
        if (check(T_SEMI)) {
            // empty return
            has_expr = false;
        } else {
            // try parse expression; if fails, record error and try to sync
            if (!Expr()) {
                add_error_line(cur.line);
                sync_to_stmt();
                // if next is semicolon consume it
                if (check(T_SEMI)) { advance(); }
                return false;
            } else {
                has_expr = true;
            }
        }

        if (!match(T_SEMI)) {
            add_error_line(cur.line);
            sync_to_stmt();
            return false;
        }

        // semantic check: match current function return type
        if (current_function.empty()) {
            // return outside function - syntax allowed? we treat as error
            add_error_line(ln);
        } else {
            auto it = func_table.find(current_function);
            if (it != func_table.end()) {
                bool returns_int = it->second.returns_int;
                if (returns_int) {
                    // int function must return expr
                    if (!has_expr) add_error_line(ln);
                } else {
                    // void function must NOT return expr
                    if (has_expr) add_error_line(ln);
                }
            } else {
                // no function info -> error
                add_error_line(ln);
            }
        }
        return true;
    }

    // assignment vs expression statement
    if (check(T_ID)) {
        // Save state for safe lookahead/backtracking
        size_t save_pos = pos;
        Token save_cur = cur;

        std::string name = cur.lexeme;
        advance();
        if (check(T_ASSIGN)) {
            // assignment
            advance();
            if (!Expr()) { add_error_line(cur.line); sync_to_stmt(); return false; }
            if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); return false; }
            // check var declared previously
            if (!vartable.has_var(name)) add_error_line(save_cur.line);
            return true;
        } else {
            // not assignment -> restore and parse as expression stmt
            pos = save_pos;
            cur = save_cur;
            if (!Expr()) { add_error_line(cur.line); sync_to_stmt(); return false; }
            if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); return false; }
            return true;
        }
    }

    // expression statement
    if (Expr()) {
        if (!match(T_SEMI)) { add_error_line(cur.line); sync_to_stmt(); return false; }
        return true;
    }

    // unknown
    add_error_line(cur.line);
    sync_to_stmt();
    return false;
}

/*
Expression parsing with precedence and left-associativity implemented iteratively
Expr -> LOrExpr
LOrExpr -> LAndExpr ( '||' LAndExpr )*
LAndExpr -> RelExpr ( '&&' RelExpr )*
RelExpr -> AddExpr ( ('<'|'>'|'<='|'>='|'=='|'!=') AddExpr )*
AddExpr -> MulExpr ( ('+'|'-') MulExpr )*
MulExpr -> UnaryExpr ( ('*'|'/'|'%') UnaryExpr )*
UnaryExpr -> ('+'|'-'| '!') UnaryExpr | PrimaryExpr
PrimaryExpr -> ID | NUMBER | '(' Expr ')' | ID '(' (Expr (',' Expr)*)? ')'
*/

bool Parser::Expr() {
    return LOrExpr();
}

bool Parser::LOrExpr() {
    if (!LAndExpr()) return false;
    while (match(T_OROR)) {
        if (!LAndExpr()) { add_error_line(cur.line); return false; }
    }
    return true;
}

bool Parser::LAndExpr() {
    if (!RelExpr()) return false;
    while (match(T_ANDAND)) {
        if (!RelExpr()) { add_error_line(cur.line); return false; }
    }
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
    while (check(T_PLUS) || check(T_MINUS)) {
        advance();
        if (!MulExpr()) { add_error_line(cur.line); return false; }
    }
    return true;
}

bool Parser::MulExpr() {
    if (!UnaryExpr()) return false;
    while (check(T_MUL) || check(T_DIV) || check(T_MOD)) {
        advance();
        if (!UnaryExpr()) { add_error_line(cur.line); return false; }
    }
    return true;
}

bool Parser::UnaryExpr() {
    if (check(T_PLUS) || check(T_MINUS) || check(T_NOT)) {
        advance();
        if (!UnaryExpr()) { add_error_line(cur.line); return false; }
        return true;
    }
    return PrimaryExpr();
}

bool Parser::PrimaryExpr() {
    if (check(T_ID)) {
        std::string name = cur.lexeme;
        int ln = cur.line;
        advance();

        // function call
        if (match(T_LPAREN)) {
            // args: optional
            if (!check(T_RPAREN)) {
                while (true) {
                    if (!Expr()) { add_error_line(cur.line); return false; }
                    if (match(T_COMMA)) continue;
                    else break;
                }
            }
            if (!match(T_RPAREN)) { add_error_line(cur.line); return false; }

            // semantic: function must be declared already or be current_function (allow recursion)
            if (!(functions_declared.count(name) || name == current_function)) {
                add_error_line(ln);
            }
            return true;
        }

        // variable usage
        if (!vartable.has_var(name)) {
            add_error_line(ln);
        }
        return true;
    }

    if (check(T_NUMBER)) {
        advance();
        return true;
    }

    if (match(T_LPAREN)) {
        if (!Expr()) { add_error_line(cur.line); return false; }
        if (!match(T_RPAREN)) { add_error_line(cur.line); return false; }
        return true;
    }

    return false;
}
