#include "parser.h"
#include "ast.h"
#include <iostream>

// ==================== 辅助方法 ====================

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

ParseError Parser::error(const Token& token, const std::string& message) {
    if (!isRecovering) {
        std::cerr << "[Error at line " << token.line << ", column " << token.column << "] "
                  << message << std::endl;
        errorCount++;
        hadError = true;
        isRecovering = true;
    }
    return ParseError(message);
}

void Parser::synchronize() {
    if (isRecovering) {
        advance();
        while (!isAtEnd()) {
            if (previous().type == TokenType::SEMICOLON) {
                isRecovering = false;
                return;
            }
            
            switch (peek(0).type) {
                case TokenType::INT:
                case TokenType::VOID:
                case TokenType::IF:
                case TokenType::ELSE:
                case TokenType::WHILE:
                case TokenType::BREAK:
                case TokenType::CONTINUE:
                case TokenType::RETURN:
                case TokenType::LBRACE:
                case TokenType::RBRACE:
                    isRecovering = false;
                    return;
            }

            advance();
        }

        isRecovering = false;
    }
}

bool Parser::isAtEnd() const {
    return peek(0).type == TokenType::END_OF_FILE;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(0), message);
}

Token Parser::previous() const {
    return tokens[current - 1];
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek(0).type == type;
}

// ==================== 解析主方法 ====================

std::shared_ptr<CompUnit> Parser::parse() {
    try {
        auto result = compUnit();
        if (hadError) {
            return nullptr;
        }
        return result;
    }
    catch (const ParseError& error) {
        return nullptr;
    }
}

// ==================== 编译单元和函数 ====================

std::shared_ptr<CompUnit> Parser::compUnit() {
    std::vector<std::shared_ptr<FunctionDef>> functions;

    int line = peek(0).line;
    int column = peek(0).column;
    
    while (!isAtEnd()) {
        isRecovering = false;

        try {
            if (check(TokenType::INT) || check(TokenType::VOID)) {
                auto func = funcDef();
                if (func) {
                    functions.push_back(func);
                }
            }
            else {
                throw error(peek(0), "Expected return type 'int' or 'void'.");
            }
        }
        catch (const ParseError& e) {
            synchronize();
            if (isAtEnd()) {
                break;
            }
        }
    }

    return std::make_shared<CompUnit>(functions, line, column);
}

std::shared_ptr<FunctionDef> Parser::funcDef() {
    int line = peek(0).line;
    int column = peek(0).column;
    
    std::string returnTypeStr;
    if (match({ TokenType::INT })) {
        returnTypeStr = "int";
    }
    else if (match({ TokenType::VOID })) {
        returnTypeStr = "void";
    }
    else {
        throw error(peek(0), "Expected return type 'int' or 'void'.");
    }

    Token nameToken = peek(0);
    try {
        nameToken = consume(TokenType::IDENTIFIER, "Expected function name.");
    }
    catch (const ParseError& e) {
        synchronize();
        return nullptr;
    }
    std::string name = nameToken.lexeme;

    try {
        consume(TokenType::LPAREN, "Expected '(' after function name.");
    }
    catch (const ParseError& e) {
        synchronize();
        if (!check(TokenType::LBRACE)) {
            return nullptr;
        }
    }

    std::vector<Param> params;

    if (check(TokenType::LBRACE)) {
        bool temp = isRecovering;
        isRecovering = false;
        error(peek(0), "Expected ')' after parameter list.");
        isRecovering = temp;
    }
    else if (!check(TokenType::RPAREN)) {
        do {
            if (!match({ TokenType::INT })) {
                if (check(TokenType::LBRACE)) {
                    bool temp = isRecovering;
                    isRecovering = false;
                    error(peek(0), "Expected ')' after parameter list.");
                    isRecovering = temp;
                    break;
                }
                throw error(peek(0), "Parameter type must be 'int'.");
            }

            try {
                Token paramName = consume(TokenType::IDENTIFIER, "Expected parameter name.");
                params.push_back(Param(paramName.lexeme));
            }
            catch (const ParseError& e) {
                synchronize();
                if (check(TokenType::LBRACE)) {
                    break;
                }
                return nullptr;
            }
        } while (match({ TokenType::COMMA }));
    }

    if (!check(TokenType::LBRACE)) {
        try {
            consume(TokenType::RPAREN, "Expected ')' after parameters.");
        }
        catch (const ParseError& e) {
            if (check(TokenType::LBRACE)) {
                synchronize();
            }
            else {
                synchronize();
                if (!check(TokenType::LBRACE)) {
                    return nullptr;
                }
            }
        }
    }

    std::shared_ptr<BlockStmt> body;
    try {
        body = block();
    }
    catch (const ParseError& e) {
        synchronize();
        return nullptr;
    }

    return std::make_shared<FunctionDef>(returnTypeStr, name, params, body, line, column);
}

Param Parser::param() {
    int line = peek(0).line;
    int column = peek(0).column;

    consume(TokenType::INT, "Parameter type must be 'int'.");
    Token name = consume(TokenType::IDENTIFIER, "Expected parameter name.");
    return Param(name.lexeme, line, column);
}

// ==================== 语句解析 ====================

std::shared_ptr<BlockStmt> Parser::block() {
    int line = peek(0).line;
    int column = peek(0).column;
    
    try {
        consume(TokenType::LBRACE, "Expected '{' before block.");
    }
    catch (const ParseError& e) {
        synchronize();
        return std::make_shared<BlockStmt>(std::vector<std::shared_ptr<Stmt>>());
    }

    std::vector<std::shared_ptr<Stmt>> statements;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        try {
            auto statement = stmt();
            if (statement) {
                statements.push_back(statement);
            }
        }
        catch (const ParseError& e) {
            synchronize();
            if (check(TokenType::RBRACE) || isAtEnd()) {
                break;
            }
        }
    }

    try {
        consume(TokenType::RBRACE, "Expected '}' after block.");
    }
    catch (const ParseError& e) {
        synchronize();
    }

    return std::make_shared<BlockStmt>(statements, line, column);
}

std::shared_ptr<Stmt> Parser::stmt() {
    if (match({TokenType::SEMICOLON})) {
        return std::make_shared<ExprStmt>(nullptr);
    }
    
    if (check(TokenType::LBRACE)) {
        return block();
    }
    
    if (match({TokenType::INT})) {
        return varDeclStmt();
    }
    
    if (match({TokenType::IF})) {
        return ifStmt();
    }
    
    if (match({TokenType::WHILE})) {
        return whileStmt();
    }
    
    if (match({TokenType::BREAK})) {
        return breakStmt();
    }
    
    if (match({TokenType::CONTINUE})) {
        return continueStmt();
    }
    
    if (match({TokenType::RETURN})) {
        return returnStmt();
    }
    
    if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::ASSIGN) {
        return assignStmt();
    }
    
    return exprStmt();
}

std::shared_ptr<Stmt> Parser::exprStmt() {
    int line = peek(0).line;
    int column = peek(0).column;
    
    auto expression = expr();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return std::make_shared<ExprStmt>(expression);
}

std::shared_ptr<Stmt> Parser::varDeclStmt() {
    int line = previous().line;
    int column = previous().column;

    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'int'.");
    consume(TokenType::ASSIGN, "Expected '=' after variable name.");
    auto initializer = expr();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    
    return std::make_shared<VarDeclStmt>(name.lexeme, initializer, line, column);
}

std::shared_ptr<Stmt> Parser::assignStmt() {
    int line = peek(0).line;
    int column = peek(0).column;

    Token name = consume(TokenType::IDENTIFIER, "Expected variable name.");
    consume(TokenType::ASSIGN, "Expected '=' after variable name.");
    auto value = expr();
    consume(TokenType::SEMICOLON, "Expected ';' after assignment.");
    
    return std::make_shared<AssignStmt>(name.lexeme, value, line, column);
}

std::shared_ptr<Stmt> Parser::ifStmt() {
    int line = previous().line;
    int column = previous().column;

    consume(TokenType::LPAREN, "Expected '(' after 'if'.");
    auto condition = expr();
    consume(TokenType::RPAREN, "Expected ')' after if condition.");
    
    auto thenBranch = stmt();
    std::shared_ptr<Stmt> elseBranch = nullptr;
    
    if (match({TokenType::ELSE})) {
        elseBranch = stmt();
    }
    
    return std::make_shared<IfStmt>(condition, thenBranch, elseBranch, line, column);
}

std::shared_ptr<Stmt> Parser::whileStmt() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::LPAREN, "Expected '(' after 'while'.");
    auto condition = expr();
    consume(TokenType::RPAREN, "Expected ')' after while condition.");
    auto body = stmt();
    
    return std::make_shared<WhileStmt>(condition, body, line, column);
}

std::shared_ptr<Stmt> Parser::breakStmt() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::SEMICOLON, "Expected ';' after 'break'.");
    return std::make_shared<BreakStmt>(line, column);
}

std::shared_ptr<Stmt> Parser::continueStmt() {
    int line = previous().line;
    int column = previous().column;
    
    consume(TokenType::SEMICOLON, "Expected ';' after 'continue'.");
    return std::make_shared<ContinueStmt>(line, column);
}

std::shared_ptr<Stmt> Parser::returnStmt() {
    int line = previous().line;
    int column = previous().column;

    std::shared_ptr<Expr> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = expr();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after return value.");
    return std::make_shared<ReturnStmt>(value, line, column);
}

// ==================== 表达式解析 ====================

std::shared_ptr<Expr> Parser::expr() {
    return lorExpr();
}

std::shared_ptr<Expr> Parser::lorExpr() {
    auto expr = landExpr();
    
    while (match({TokenType::OR})) {
        std::string op = previous().lexeme;
        auto right = landExpr();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::landExpr() {
    auto expr = relExpr();
    
    while (match({TokenType::AND})) {
        std::string op = previous().lexeme;
        auto right = relExpr();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::relExpr() {
    auto expr = addExpr();
    
    while (match({TokenType::LT, TokenType::GT, TokenType::LE, 
                 TokenType::GE, TokenType::EQ, TokenType::NEQ})) {
        std::string op = previous().lexeme;
        auto right = addExpr();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::addExpr() {
    auto expr = mulExpr();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        std::string op = previous().lexeme;
        auto right = mulExpr();
        expr = std::make_shared<BinaryExpr>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::mulExpr() {
    auto expr = unaryExpr();
    
    while (match({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO})) {
        std::string op = previous().lexeme;
        int line = previous().line;
        int column = previous().column;
        auto right = unaryExpr();
        expr = std::make_shared<BinaryExpr>(expr, op, right, line, column);
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::unaryExpr() {
    if (match({TokenType::PLUS, TokenType::MINUS, TokenType::NOT})) {
        std::string op = previous().lexeme;
        int line = previous().line;
        int column = previous().column;
        auto right = unaryExpr();
        return std::make_shared<UnaryExpr>(op, right, line, column);
    }
    
    return primaryExpr();
}

std::shared_ptr<Expr> Parser::primaryExpr() {
    if (match({TokenType::NUMBER})) {
        int value = std::stoi(previous().lexeme);
        int line = previous().line;
        int column = previous().column;
        return std::make_shared<NumberExpr>(value, line, column);
    }
    
    if (match({TokenType::IDENTIFIER})) {
        std::string name = previous().lexeme;
        int line = previous().line;
        int column = previous().column;
        
        if (match({TokenType::LPAREN})) {
            std::vector<std::shared_ptr<Expr>> arguments;
            
            if (!check(TokenType::RPAREN)) {
                do {
                    arguments.push_back(expr());
                } while (match({TokenType::COMMA}));
            }
            
            consume(TokenType::RPAREN, "Expected ')' after arguments.");
            
            return std::make_shared<CallExpr>(name, arguments, line, column);
        }
        
        return std::make_shared<VariableExpr>(name, line, column);
    }
    
    if (match({TokenType::LPAREN})) {
        auto expression = expr();
        consume(TokenType::RPAREN, "Expected ')' after expression.");
        return expression;
    }
    
    throw error(peek(0), "Expected expression.");
}