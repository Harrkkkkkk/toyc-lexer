#ifndef TOYC_LEXER_H
#define TOYC_LEXER_H

#include "token.h"
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(const std::string &src);
    std::vector<Token> tokenize();

private:
    const std::string text;
    size_t i;
    int line;

    char peek() const;
    char get();
    bool starts_with(const std::string &s) const;
    void skip_whitespace_and_comments();
    Token lex_identifier_or_keyword();
    Token lex_number();
};

#endif
