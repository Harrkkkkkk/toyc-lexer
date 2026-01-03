#include "lexer.h"
#include <cctype>
#include <iostream>
#include <stdexcept>

// ==================== 构造函数 ====================

Lexer::Lexer() : source(""), position(0), line(1), column(1) {
    keywords["int"] = TokenType::INT;
    keywords["void"] = TokenType::VOID;
    keywords["if"] = TokenType::IF;
    keywords["else"] = TokenType::ELSE;
    keywords["while"] = TokenType::WHILE;
    keywords["break"] = TokenType::BREAK;
    keywords["continue"] = TokenType::CONTINUE;
    keywords["return"] = TokenType::RETURN;
    
    initOperators();
}

Lexer::Lexer(const std::string& source) : source(source), position(0), line(1), column(1) {
    keywords["int"] = TokenType::INT;
    keywords["void"] = TokenType::VOID;
    keywords["if"] = TokenType::IF;
    keywords["else"] = TokenType::ELSE;
    keywords["while"] = TokenType::WHILE;
    keywords["break"] = TokenType::BREAK;
    keywords["continue"] = TokenType::CONTINUE;
    keywords["return"] = TokenType::RETURN;

    initOperators();
}

// ==================== 核心扫描方法 ====================

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;
        
        tokens.push_back(scanToken());
    }
    
    tokens.push_back(Token(TokenType::END_OF_FILE, "", line, column));
    return tokens;
}

Token Lexer::scanToken() {
    char c = advance();
    
    if (isalpha(c) || c == '_') {
        position--;
        column--;
        return scanIdentifier();
    }
    
    if (isdigit(c)) {
        position--;
        column--;
        return scanNumber();
    }
    
    int tokenLine = line;
    int tokenColumn = column - 1;

    switch (c) {
        case '(': return Token(TokenType::LPAREN, "(", tokenLine, tokenColumn);
        case ')': return Token(TokenType::RPAREN, ")", tokenLine, tokenColumn);
        case '{': return Token(TokenType::LBRACE, "{", tokenLine, tokenColumn);
        case '}': return Token(TokenType::RBRACE, "}", tokenLine, tokenColumn);
        case ';': return Token(TokenType::SEMICOLON, ";", tokenLine, tokenColumn);
        case ',': return Token(TokenType::COMMA, ",", tokenLine, tokenColumn);
        case '+': return Token(TokenType::PLUS, "+", tokenLine, tokenColumn);
        case '-': return Token(TokenType::MINUS, "-", tokenLine, tokenColumn);
        case '*': return Token(TokenType::MULTIPLY, "*", tokenLine, tokenColumn);
        case '%': return Token(TokenType::MODULO, "%", tokenLine, tokenColumn);
        case '/': return Token(TokenType::DIVIDE, "/", tokenLine, tokenColumn);
            
        case '=':
            if (peek() == '=') {
                advance();
                return Token(TokenType::EQ, "==", tokenLine, tokenColumn);
            }
            return Token(TokenType::ASSIGN, "=", tokenLine, tokenColumn);
            
        case '!':
            if (peek() == '=') {
                advance();
                return Token(TokenType::NEQ, "!=", tokenLine, tokenColumn);
            }
            return Token(TokenType::NOT, "!", tokenLine, tokenColumn);
            
        case '<':
            if (peek() == '=') {
                advance();
                return Token(TokenType::LE, "<=", tokenLine, tokenColumn);
            }
            return Token(TokenType::LT, "<", tokenLine, tokenColumn);
            
        case '>':
            if (peek() == '=') {
                advance();
                return Token(TokenType::GE, ">=", tokenLine, tokenColumn);
            }
            return Token(TokenType::GT, ">", tokenLine, tokenColumn);
            
        case '&':
            if (peek() == '&') {
                advance();
                return Token(TokenType::AND, "&&", tokenLine, tokenColumn);
            }
            return Token(TokenType::UNKNOWN, "&", tokenLine, tokenColumn);
            
        case '|':
            if (peek() == '|') {
                advance();
                return Token(TokenType::OR, "||", tokenLine, tokenColumn);
            }
            return Token(TokenType::UNKNOWN, "|", tokenLine, tokenColumn);
    }
    
    return Token(TokenType::UNKNOWN, std::string(1, c), tokenLine, tokenColumn);
}

Token Lexer::scanIdentifier() {
    int startPos = position;
    int startColumn = column;
    int startLine = line;
    
    if (isalpha(peek()) || peek() == '_') {
        advance();
    }
    
    while (!isAtEnd() && (isalnum(peek()) || peek() == '_')) {
        advance();
    }

    std::string lexeme = source.substr(startPos, position - startPos);
    
    if (keywords.find(lexeme) != keywords.end()) {
        return Token(keywords[lexeme], lexeme, startLine, startColumn);
    }
    
    return Token(TokenType::IDENTIFIER, lexeme, startLine, startColumn);
}

Token Lexer::scanNumber() {
    int startPos = position;
    int startColumn = column;
    int startLine = line;
    
    if (peek() == '-') {
        advance();
    }

    while (!isAtEnd() && isdigit(peek())) {
        advance();
    }
    
    std::string lexeme = source.substr(startPos, position - startPos);
    return Token(TokenType::NUMBER, lexeme, startLine, startColumn);
}

// ==================== 辅助方法 ====================

void Lexer::initOperators() {
    operators["="] = TokenType::ASSIGN;
    operators["+"] = TokenType::PLUS;
    operators["-"] = TokenType::MINUS;
    operators["*"] = TokenType::MULTIPLY;
    operators["/"] = TokenType::DIVIDE;
    operators["%"] = TokenType::MODULO;
    operators["<"] = TokenType::LT;
    operators[">"] = TokenType::GT;
    operators["<="] = TokenType::LE;
    operators[">="] = TokenType::GE;
    operators["=="] = TokenType::EQ;
    operators["!="] = TokenType::NEQ;
    operators["&&"] = TokenType::AND;
    operators["||"] = TokenType::OR;
    operators["!"] = TokenType::NOT;
    operators["("] = TokenType::LPAREN;
    operators[")"] = TokenType::RPAREN;
    operators["{"] = TokenType::LBRACE;
    operators["}"] = TokenType::RBRACE;
    operators[";"] = TokenType::SEMICOLON;
    operators[","] = TokenType::COMMA;
}

char Lexer::peek(int offset) const {
    if (position + offset >= source.length()) {
        return '\0';
    }
    return source[position + offset];
}

char Lexer::advance() {
    char current = source[position++];
    
    if (current == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    
    return current;
}

bool Lexer::isAtEnd() const {
    return position >= source.length();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case '\n':
                advance();
                break;
            case '/':
                if (peek(1) == '/' || peek(1) == '*') {
                    skipComment();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

void Lexer::skipComment() {
    char c = peek();
    
    if (c == '/') {
        if (peek(1) == '/') {
            position += 2;
            column += 2;
            
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else if (peek(1) == '*') {
            position += 2;
            column += 2;
            
            while (!isAtEnd()) {
                if (peek() == '*' && peek(1) == '/') {
                    position += 2;
                    column += 2;
                    break;
                }
                if (peek() == '\n') {
                    line++;
                    column = 1;
                    position++;
                } else {
                    position++;
                    column++;
                }
            }
        }
    }
}

Token Lexer::readOperatorOrPunctuator() {
    int start = position;
    int startLine = line;
    int startColumn = column;

    char c = peek();
    position++;
    column++;

    if (position < source.length()) {
        char next = peek();
        std::string twoCharOp = std::string(1, c) + next;
        
        if (operators.find(twoCharOp) != operators.end()) {
            position++;
            column++;
            return Token(operators.at(twoCharOp), twoCharOp, startLine, startColumn);
        }
    }

    std::string singleCharOp = std::string(1, c);
    if (operators.find(singleCharOp) != operators.end()) {
        return Token(operators.at(singleCharOp), singleCharOp, startLine, startColumn);
    }

    return Token(TokenType::UNKNOWN, singleCharOp, startLine, startColumn);
}

// ==================== 公共接口 ====================

Token Lexer::nextToken() {
    while (true) {
        skipWhitespace();
        
        if (isAtEnd()) {
            return Token(TokenType::END_OF_FILE, "", line, column);
        }
        
        return scanToken();
    }
}

Token Lexer::peekToken() {
    int savedPosition = position;
    int savedLine = line;
    int savedColumn = column;

    Token token = nextToken();

    position = savedPosition;
    line = savedLine;
    column = savedColumn;

    return token;
}

std::vector<Token> Lexer::tokenize(const std::string& source) {
    Lexer lexer(source);
    return lexer.tokenize();
}