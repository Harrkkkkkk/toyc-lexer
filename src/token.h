#ifndef TOYC_TOKEN_H
#define TOYC_TOKEN_H

#include <string>

enum TokenType {
    T_EOF,
    T_ID,
    T_NUMBER,
    // keywords
    T_KW_INT,
    T_KW_VOID,
    T_KW_IF,
    T_KW_ELSE,
    T_KW_WHILE,
    T_KW_BREAK,
    T_KW_CONTINUE,
    T_KW_RETURN,
    // punctuators / ops
    T_PLUS, T_MINUS, T_MUL, T_DIV, T_MOD,
    T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_COMMA, T_SEMI,
    T_ASSIGN, // =
    T_EQ, T_NEQ, T_LT, T_GT, T_LE, T_GE,
    T_ANDAND, T_OROR, T_NOT,
    T_UNKNOWN
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    Token(TokenType t=T_UNKNOWN, std::string l="", int ln=1): type(t), lexeme(std::move(l)), line(ln) {}
};

#endif
