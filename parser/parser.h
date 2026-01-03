#pragma once
#include "lexer/lexer.h"
#include "parser/ast.h"
#include <vector>
#include <memory>
#include <stdexcept>

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message) : std::runtime_error(message) {}
};

class Parser {
private:
    std::vector<Token> tokens;
    int current = 0;
    bool hadError = false;
    int errorCount = 0;
    bool isRecovering = false;

public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens) {}
    
    std::shared_ptr<CompUnit> parse();
    bool hasError() const { return hadError; }

private:
    Token peek(int offset) const {
        if (current + offset >= tokens.size()) {
            return tokens.back();
        }
        return tokens[current + offset];
    }
    
    Token previous() const;
    bool isAtEnd() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    ParseError error(const Token& token, const std::string& message);
    void synchronize();

    std::shared_ptr<CompUnit> compUnit();
    std::shared_ptr<FunctionDef> funcDef();
    Param param();
    std::shared_ptr<Stmt> stmt();
    std::shared_ptr<BlockStmt> block();
    std::shared_ptr<Stmt> exprStmt();
    std::shared_ptr<Stmt> varDeclStmt();
    std::shared_ptr<Stmt> assignStmt();
    std::shared_ptr<Stmt> ifStmt();
    std::shared_ptr<Stmt> whileStmt();
    std::shared_ptr<Stmt> breakStmt();
    std::shared_ptr<Stmt> continueStmt();
    std::shared_ptr<Stmt> returnStmt();
    
    std::shared_ptr<Expr> expr();
    std::shared_ptr<Expr> lorExpr();
    std::shared_ptr<Expr> landExpr();
    std::shared_ptr<Expr> relExpr();
    std::shared_ptr<Expr> addExpr();
    std::shared_ptr<Expr> mulExpr();
    std::shared_ptr<Expr> unaryExpr();
    std::shared_ptr<Expr> primaryExpr();
};