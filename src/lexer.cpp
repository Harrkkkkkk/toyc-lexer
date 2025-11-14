#include "lexer.h"
#include <cctype>

Lexer::Lexer(const std::string &src_): text(src_), i(0), line(1) {}

char Lexer::peek() const {
    if (i >= text.size()) return '\0';
    return text[i];
}
char Lexer::get() {
    if (i >= text.size()) return '\0';
    char c = text[i++];
    if (c == '\n') ++line;
    return c;
}
bool Lexer::starts_with(const std::string &s) const {
    if (i + s.size() > text.size()) return false;
    return text.compare(i, s.size(), s) == 0;
}

void Lexer::skip_whitespace_and_comments() {
    while (true) {
        if (std::isspace((unsigned char)peek())) { get(); continue; }
        if (starts_with("//")) {
            // single-line comment
            i += 2;
            while (peek() != '\n' && peek() != '\0') get();
            continue;
        }
        if (starts_with("/*")) {
            i += 2;
            bool closed = false;
            while (peek() != '\0') {
                if (starts_with("*/")) { i += 2; closed = true; break; }
                get();
            }
            if (!closed) {
                // unterminated comment -> we'll treat as EOF but inject unknown token handled by parser
                return;
            }
            continue;
        }
        break;
    }
}

Token Lexer::lex_identifier_or_keyword() {
    std::string s;
    while (std::isalnum((unsigned char)peek()) || peek() == '_') s.push_back(get());
    if (s == "int") return Token(T_KW_INT, s, line);
    if (s == "void") return Token(T_KW_VOID, s, line);
    if (s == "if") return Token(T_KW_IF, s, line);
    if (s == "else") return Token(T_KW_ELSE, s, line);
    if (s == "while") return Token(T_KW_WHILE, s, line);
    if (s == "break") return Token(T_KW_BREAK, s, line);
    if (s == "continue") return Token(T_KW_CONTINUE, s, line);
    if (s == "return") return Token(T_KW_RETURN, s, line);
    return Token(T_ID, s, line);
}

Token Lexer::lex_number() {
    std::string s;
    if (peek() == '-') s.push_back(get()); // spec allows -? leading
    if (peek() == '0') {
        s.push_back(get());
    } else {
        if (!std::isdigit((unsigned char)peek())) {
            // just '-' not followed by digit -> return minus as token handled outside
            return Token(T_UNKNOWN, s, line);
        }
        while (std::isdigit((unsigned char)peek())) s.push_back(get());
    }
    return Token(T_NUMBER, s, line);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> out;
    while (peek() != '\0') {
        skip_whitespace_and_comments();
        char c = peek();
        if (c == '\0') break;
        // multi-char ops
        if (starts_with("==")) { out.emplace_back(T_EQ, "==", line); i += 2; continue; }
        if (starts_with("!=")) { out.emplace_back(T_NEQ, "!=", line); i += 2; continue; }
        if (starts_with("<=")) { out.emplace_back(T_LE, "<=", line); i += 2; continue; }
        if (starts_with(">=")) { out.emplace_back(T_GE, ">=", line); i += 2; continue; }
        if (starts_with("&&")) { out.emplace_back(T_ANDAND, "&&", line); i += 2; continue; }
        if (starts_with("||")) { out.emplace_back(T_OROR, "||", line); i += 2; continue; }

        if (std::isalpha((unsigned char)c) || c == '_') {
            out.push_back(lex_identifier_or_keyword()); continue;
        }
        if (std::isdigit((unsigned char)c) || (c == '-' && i+1 < text.size() && std::isdigit((unsigned char)text[i+1]))) {
            out.push_back(lex_number()); continue;
        }

        // single char
        get(); // consume
        switch (c) {
            case '+': out.emplace_back(T_PLUS, "+", line); break;
            case '-': out.emplace_back(T_MINUS, "-", line); break;
            case '*': out.emplace_back(T_MUL, "*", line); break;
            case '/': out.emplace_back(T_DIV, "/", line); break;
            case '%': out.emplace_back(T_MOD, "%", line); break;
            case '(' : out.emplace_back(T_LPAREN, "(", line); break;
            case ')' : out.emplace_back(T_RPAREN, ")", line); break;
            case '{' : out.emplace_back(T_LBRACE, "{", line); break;
            case '}' : out.emplace_back(T_RBRACE, "}", line); break;
            case ',' : out.emplace_back(T_COMMA, ",", line); break;
            case ';' : out.emplace_back(T_SEMI, ";", line); break;
            case '=' : out.emplace_back(T_ASSIGN, "=", line); break;
            case '<' : out.emplace_back(T_LT, "<", line); break;
            case '>' : out.emplace_back(T_GT, ">", line); break;
            case '!' : out.emplace_back(T_NOT, "!", line); break;
            case '&' : out.emplace_back(T_UNKNOWN, "&", line); break;
            case '|' : out.emplace_back(T_UNKNOWN, "|", line); break;
            default:
                out.emplace_back(T_UNKNOWN, std::string(1,c), line);
        }
    }
    out.emplace_back(T_EOF, "", line);
    return out;
}
