#include <iostream>
#include <string>
#include <cctype>
#include <unordered_map>

enum class TokenType {
    KW_INT, KW_VOID, KW_IF, KW_ELSE, KW_WHILE,
    KW_BREAK, KW_CONTINUE, KW_RETURN,
    LPAREN, RPAREN, LBRACE, RBRACE,
    SEMICOLON, COMMA, ASSIGN,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    LT, GT, LE, GE, EQ, NE,
    AND, OR,
    IDENT, INT_CONST
};

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KW_INT: return "'int'";
        case TokenType::KW_VOID: return "'void'";
        case TokenType::KW_IF: return "'if'";
        case TokenType::KW_ELSE: return "'else'";
        case TokenType::KW_WHILE: return "'while'";
        case TokenType::KW_BREAK: return "'break'";
        case TokenType::KW_CONTINUE: return "'continue'";
        case TokenType::KW_RETURN: return "'return'";
        case TokenType::LPAREN: return "'('";
        case TokenType::RPAREN: return "')'";
        case TokenType::LBRACE: return "'{'";
        case TokenType::RBRACE: return "'}'";
        case TokenType::SEMICOLON: return "';'";
        case TokenType::COMMA: return "','";
        case TokenType::ASSIGN: return "'='";
        case TokenType::PLUS: return "'+'";
        case TokenType::MINUS: return "'-'";
        case TokenType::STAR: return "'*'";
        case TokenType::SLASH: return "'/'";
        case TokenType::PERCENT: return "'%'";
        case TokenType::LT: return "'<'";
        case TokenType::GT: return "'>'";
        case TokenType::LE: return "'<='";
        case TokenType::GE: return "'>='";
        case TokenType::EQ: return "'=='";
        case TokenType::NE: return "'!='";
        case TokenType::AND: return "'&&'";
        case TokenType::OR: return "'||'";
        case TokenType::IDENT: return "Ident";
        case TokenType::INT_CONST: return "IntConst";
        default: return "Unknown";
    }
}

class Lexer {
private:
    std::string input;
    size_t pos;
    int token_id;

    char peek(int offset = 0) const {
        if (pos + offset >= input.size()) return '\0';
        return input[pos + offset];
    }

    void advance() {
        pos++;
    }

    void skipWhitespace() {
        while (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r') {
            advance();
        }
    }

    void skipComment() {
        if (peek() == '/') {
            if (peek(1) == '/') {
                // 单行注释
                while (peek() != '\0' && peek() != '\n') {
                    advance();
                }
            } else if (peek(1) == '*') {
                // 多行注释
                advance(); advance(); // skip /*
                while (peek() != '\0') {
                    if (peek() == '*' && peek(1) == '/') {
                        advance(); advance(); // skip */
                        break;
                    }
                    advance();
                }
            }
        }
    }

    std::string readIdentifier() {
        std::string id;
        while (isalnum(peek()) || peek() == '_') {
            id += peek();
            advance();
        }
        return id;
    }

    std::string readNumber() {
        std::string num;
        if (peek() == '-') {
            num += peek();
            advance();
        }
        while (isdigit(peek())) {
            num += peek();
            advance();
        }
        return num;
    }

public:
    Lexer() : pos(0), token_id(0) {
        char c;
        while (std::cin.get(c)) {
            input += c;
        }
    }

    bool hasNext() const {
        return pos < input.size();
    }

    std::pair<TokenType, std::string> nextToken() {
        skipWhitespace();
        if (peek() == '/' && (peek(1) == '/' || peek(1) == '*')) {
            skipComment();
            return nextToken();
        }

        if (peek() == '\0') {
            return {TokenType::INT_CONST, ""}; // Should not happen
        }

        char ch = peek();
        advance();

        // Handle multi-char tokens
        if (ch == '=') {
            if (peek() == '=') { advance(); return {TokenType::EQ, "=="}; }
            return {TokenType::ASSIGN, "="};
        }
        if (ch == '<') {
            if (peek() == '=') { advance(); return {TokenType::LE, "<="}; }
            return {TokenType::LT, "<"};
        }
        if (ch == '>') {
            if (peek() == '=') { advance(); return {TokenType::GE, ">="}; }
            return {TokenType::GT, ">"};
        }
        if (ch == '!') {
            if (peek() == '=') { advance(); return {TokenType::NE, "!="}; }
            // Invalid '!' alone, but we treat as error later
        }
        if (ch == '&') {
            if (peek() == '&') { advance(); return {TokenType::AND, "&&"}; }
        }
        if (ch == '|') {
            if (peek() == '|') { advance(); return {TokenType::OR, "||"}; }
        }

        // Single-char tokens
        switch (ch) {
            case '(': return {TokenType::LPAREN, "("};
            case ')': return {TokenType::RPAREN, ")"};
            case '{': return {TokenType::LBRACE, "{"};
            case '}': return {TokenType::RBRACE, "}"};
            case ';': return {TokenType::SEMICOLON, ";"};
            case ',': return {TokenType::COMMA, ","};
            case '+': return {TokenType::PLUS, "+"};
            case '*': return {TokenType::STAR, "*"};
            case '/': return {TokenType::SLASH, "/"};
            case '%': return {TokenType::PERCENT, "%"};
        }

        // Identifiers and keywords
        if (isalpha(ch) || ch == '_') {
            std::string ident(1, ch);
            ident += readIdentifier();
            static const std::unordered_map<std::string, TokenType> keywords = {
                {"int", TokenType::KW_INT},
                {"void", TokenType::KW_VOID},
                {"if", TokenType::KW_IF},
                {"else", TokenType::KW_ELSE},
                {"while", TokenType::KW_WHILE},
                {"break", TokenType::KW_BREAK},
                {"continue", TokenType::KW_CONTINUE},
                {"return", TokenType::KW_RETURN}
            };
            auto it = keywords.find(ident);
            if (it != keywords.end()) {
                return {it->second, ident};
            }
            return {TokenType::IDENT, ident};
        }

        // Numbers
        if (isdigit(ch) || ch == '-') {
            std::string num(1, ch);
            if (ch == '-') {
                if (isdigit(peek())) {
                    num += readNumber();
                } else {
                    // Standalone '-' is handled above
                    return {TokenType::MINUS, "-"};
                }
            } else {
                num += readNumber();
            }
            return {TokenType::INT_CONST, num};
        }

        // Fallback for '-'
        if (ch == '-') {
            return {TokenType::MINUS, "-"};
        }

        // Should not reach here in valid input
        return {TokenType::INT_CONST, std::string(1, ch)};
    }

    int getCurrentId() const { return token_id; }
    void incrementId() { token_id++; }
};

int main() {
    Lexer lexer;
    int id = 0;
    while (lexer.hasNext()) {
        auto [type, value] = lexer.nextToken();
        if (value.empty() && type == TokenType::INT_CONST) break; // End of input
        std::cout << id << ":" << tokenTypeToString(type) << ":\"" << value << "\"\n";
        id++;
    }
    return 0;
}