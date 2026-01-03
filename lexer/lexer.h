#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// 标记类型枚举 - 定义了所有可能的标记类型
enum class TokenType {
    // 关键字
    INT, VOID, IF, ELSE, WHILE, BREAK, CONTINUE, RETURN,
    
    // 标识符和字面量
    IDENTIFIER, NUMBER,
    
    // 运算符
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    ASSIGN, 
    EQ, NEQ, LT, GT, LE, GE,
    AND, OR, NOT,
    
    // 分隔符
    LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON, COMMA,
    
    // 其他
    END_OF_FILE, UNKNOWN
};

// 标记结构体 - 表示源代码中的一个词法单元
struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    
    Token(TokenType type, const std::string& lexeme, int line, int column)
        : type(type), lexeme(lexeme), line(line), column(column) {}
};

// Lexer类 - 负责将源代码字符串分解为标记序列
class Lexer {
private:
    std::string source;
    int position = 0;
    int line = 1;
    int column = 1;
    
    std::unordered_map<std::string, TokenType> keywords;
    std::unordered_map<std::string, TokenType> operators;

    void initOperators();
    char peek(int offset = 0) const;
    char advance();
    bool isAtEnd() const;
    void skipWhitespace();
    void skipComment();
    
    Token scanToken();
    Token scanIdentifier();
    Token scanNumber();
    Token readOperatorOrPunctuator();

public:
    Lexer();
    Lexer(const std::string& source);
    
    int getLine() const { return line; }
    int getColumn() const { return column; }
    
    Token nextToken();
    Token peekToken();
    
    std::vector<Token> tokenize();
    std::vector<Token> tokenize(const std::string& source);
};