#pragma once

#include <string>
#include <vector>
#include <iostream>

// Token类型枚举
enum class TokenType {
    // 字面量
    NUMBER,
    IDENTIFIER,
    // 关键字
    INT, VOID, IF, ELSE, WHILE, BREAK, CONTINUE, RETURN,
    // 运算符
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    LT, GT, LE, GE, EQ, NE,
    AND, OR, NOT,
    ASSIGN,
    // 分隔符
    LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON, COMMA,

    EOF_TOKEN, ERROR_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token(TokenType t, const std::string& v, int l = 1, int c = 1)
        : type(t), value(v), line(l), column(c) {
    }
};

// 词法分析器
class Lexer {
public:
    explicit Lexer(const std::string& source);

    Token next_token();
    std::vector<Token> tokenize();

    static std::string token_type_to_string(TokenType type);

private:
    char current_char();
    char peek_char(size_t offset = 1);
    void advance();
    void skip_whitespace();
    void skip_comment();

    Token read_number();
    Token read_identifier();
    Token read_string();

    bool is_alpha(char c);
    bool is_digit(char c);
    bool is_alnum(char c);

    TokenType get_keyword_type(const std::string& word);

private:
    std::string input;
    size_t position;
    int line;
    int column;
};