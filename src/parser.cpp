#include "parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& token_list) 
    : tokens(token_list), current_token_index(0), has_error(false) {}

std::unique_ptr<CompUnit> Parser::parse() {
    return parse_comp_unit();
}

void Parser::report_error(const std::string& message) {
    has_error = true;
    error_message = "Parse error at line " + std::to_string(current_token().line) + 
                   ", column " + std::to_string(current_token().column) + ": " + message;
    std::cerr << error_message << std::endl;
}

const Token& Parser::current_token() {
    if (current_token_index >= tokens.size()) {
        static Token eof_token(TokenType::EOF_TOKEN, "");
        return eof_token;
    }
    return tokens[current_token_index];
}

const Token& Parser::peek_token(size_t offset) {
    size_t peek_index = current_token_index + offset;
    if (peek_index >= tokens.size()) {
        static Token eof_token(TokenType::EOF_TOKEN, "");
        return eof_token;
    }
    return tokens[peek_index];
}

void Parser::advance_token() {
    if (current_token_index < tokens.size()) {
        current_token_index++;
    }
}

bool Parser::match(TokenType type) {
    return current_token().type == type;
}

bool Parser::consume(TokenType type, const std::string& error_msg) {
    if (match(type)) {
        advance_token();
        return true;
    } else {
        if (!error_msg.empty()) {
            report_error(error_msg);
        }
        return false;
    }
}

std::unique_ptr<CompUnit> Parser::parse_comp_unit() {
    auto comp_unit = std::make_unique<CompUnit>();
    
    while (!match(TokenType::EOF_TOKEN) && !has_error) {
        auto func = parse_func_def();
        if (func) {
            comp_unit->functions.push_back(std::move(func));
        } else {
            break;
        }
    }
    
    return comp_unit;
}

std::unique_ptr<FuncDef> Parser::parse_func_def() {
    // 解析返回类型
    FuncDef::ReturnType return_type;
    if (match(TokenType::INT)) {
        return_type = FuncDef::ReturnType::INT;
        advance_token();
    } else if (match(TokenType::VOID)) {
        return_type = FuncDef::ReturnType::VOID;
        advance_token();
    } else {
        report_error("Expected 'int' or 'void'");
        return nullptr;
    }
    
    // 解析函数名
    if (!match(TokenType::IDENTIFIER)) {
        report_error("Expected function name");
        return nullptr;
    }
    std::string func_name = current_token().value;
    advance_token();
    
    // 解析参数列表
    if (!consume(TokenType::LPAREN, "Expected '('")) {
        return nullptr;
    }
    
    std::vector<std::string> params = parse_param_list();
    
    if (!consume(TokenType::RPAREN, "Expected ')'")) {
        return nullptr;
    }
    
    // 解析函数体
    auto body = parse_block();
    if (!body) {
        return nullptr;
    }
    
    return std::make_unique<FuncDef>(return_type, func_name, std::move(params), std::move(body));
}

std::vector<std::string> Parser::parse_param_list() {
    std::vector<std::string> params;
    
    if (match(TokenType::RPAREN)) {
        // 空参数列表
        return params;
    }
    
    // 第一个参数
    if (!consume(TokenType::INT, "Expected 'int'")) {
        return params;
    }
    if (!match(TokenType::IDENTIFIER)) {
        report_error("Expected parameter name");
        return params;
    }
    params.push_back(current_token().value);
    advance_token();
    
    // 后续参数
    while (match(TokenType::COMMA)) {
        advance_token(); // 跳过逗号
        
        if (!consume(TokenType::INT, "Expected 'int'")) {
            break;
        }
        if (!match(TokenType::IDENTIFIER)) {
            report_error("Expected parameter name");
            break;
        }
        params.push_back(current_token().value);
        advance_token();
    }
    
    return params;
}

std::unique_ptr<Block> Parser::parse_block() {
    if (!consume(TokenType::LBRACE, "Expected '{'")) {
        return nullptr;
    }
    
    auto block = std::make_unique<Block>();
    
    while (!match(TokenType::RBRACE) && !match(TokenType::EOF_TOKEN) && !has_error) {
        auto stmt = parse_stmt();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        } else {
            break;
        }
    }
    
    if (!consume(TokenType::RBRACE, "Expected '}'")) {
        return nullptr;
    }
    
    return block;
}

std::unique_ptr<Stmt> Parser::parse_stmt() {
    // Block
    if (match(TokenType::LBRACE)) {
        return parse_block();
    }
    
    // 空语句
    if (match(TokenType::SEMICOLON)) {
        advance_token();
        return std::make_unique<ExprStmt>(nullptr);
    }
    
    // if语句
    if (match(TokenType::IF)) {
        advance_token();
        if (!consume(TokenType::LPAREN, "Expected '(' after 'if'")) {
            return nullptr;
        }
        
        auto condition = parse_expr();
        if (!condition) {
            return nullptr;
        }
        
        if (!consume(TokenType::RPAREN, "Expected ')' after if condition")) {
            return nullptr;
        }
        
        auto then_stmt = parse_stmt();
        if (!then_stmt) {
            return nullptr;
        }
        
        std::unique_ptr<Stmt> else_stmt = nullptr;
        if (match(TokenType::ELSE)) {
            advance_token();
            else_stmt = parse_stmt();
        }
        
        return std::make_unique<IfStmt>(std::move(condition), std::move(then_stmt), std::move(else_stmt));
    }
    
    // while语句
    if (match(TokenType::WHILE)) {
        advance_token();
        if (!consume(TokenType::LPAREN, "Expected '(' after 'while'")) {
            return nullptr;
        }
        
        auto condition = parse_expr();
        if (!condition) {
            return nullptr;
        }
        
        if (!consume(TokenType::RPAREN, "Expected ')' after while condition")) {
            return nullptr;
        }
        
        auto body = parse_stmt();
        if (!body) {
            return nullptr;
        }
        
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }
    
    // break语句
    if (match(TokenType::BREAK)) {
        advance_token();
        if (!consume(TokenType::SEMICOLON, "Expected ';' after 'break'")) {
            return nullptr;
        }
        return std::make_unique<BreakStmt>();
    }
    
    // continue语句
    if (match(TokenType::CONTINUE)) {
        advance_token();
        if (!consume(TokenType::SEMICOLON, "Expected ';' after 'continue'")) {
            return nullptr;
        }
        return std::make_unique<ContinueStmt>();
    }
    
    // return语句
    if (match(TokenType::RETURN)) {
        advance_token();
        
        if (match(TokenType::SEMICOLON)) {
            advance_token();
            return std::make_unique<ReturnStmt>(nullptr);
        } else {
            auto expr = parse_expr();
            if (!expr) {
                return nullptr;
            }
            if (!consume(TokenType::SEMICOLON, "Expected ';' after return expression")) {
                return nullptr;
            }
            return std::make_unique<ReturnStmt>(std::move(expr));
        }
    }
    
    // 变量声明或赋值语句
    if (match(TokenType::INT)) {
        advance_token();
        if (!match(TokenType::IDENTIFIER)) {
            report_error("Expected variable name");
            return nullptr;
        }
        
        std::string var_name = current_token().value;
        advance_token();
        
        if (!consume(TokenType::ASSIGN, "Expected '=' in variable declaration")) {
            return nullptr;
        }
        
        auto init_expr = parse_expr();
        if (!init_expr) {
            return nullptr;
        }
        
        if (!consume(TokenType::SEMICOLON, "Expected ';' after variable declaration")) {
            return nullptr;
        }
        
        return std::make_unique<VarDecl>(var_name, std::move(init_expr));
    }
    
    // 赋值语句或表达式语句
    if (match(TokenType::IDENTIFIER)) {
        std::string name = current_token().value;
        advance_token();
        
        if (match(TokenType::ASSIGN)) {
            advance_token();
            auto expr = parse_expr();
            if (!expr) {
                return nullptr;
            }
            if (!consume(TokenType::SEMICOLON, "Expected ';' after assignment")) {
                return nullptr;
            }
            return std::make_unique<AssignStmt>(name, std::move(expr));
        } else {
            // 回退，这是一个表达式语句
            current_token_index--;
            auto expr = parse_expr();
            if (!expr) {
                return nullptr;
            }
            if (!consume(TokenType::SEMICOLON, "Expected ';' after expression")) {
                return nullptr;
            }
            return std::make_unique<ExprStmt>(std::move(expr));
        }
    }
    
    // 表达式语句
    auto expr = parse_expr();
    if (expr) {
        if (!consume(TokenType::SEMICOLON, "Expected ';' after expression")) {
            return nullptr;
        }
        return std::make_unique<ExprStmt>(std::move(expr));
    }
    
    return nullptr;
}

std::unique_ptr<Expr> Parser::parse_expr() {
    return parse_lor_expr();
}

std::unique_ptr<Expr> Parser::parse_lor_expr() {
    auto left = parse_land_expr();
    if (!left) return nullptr;
    
    while (match(TokenType::OR)) {
        advance_token();
        auto right = parse_land_expr();
        if (!right) return nullptr;
        
        left = std::make_unique<BinaryExpr>(BinaryExpr::Operator::OR, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parse_land_expr() {
    auto left = parse_rel_expr();
    if (!left) return nullptr;
    
    while (match(TokenType::AND)) {
        advance_token();
        auto right = parse_rel_expr();
        if (!right) return nullptr;
        
        left = std::make_unique<BinaryExpr>(BinaryExpr::Operator::AND, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parse_rel_expr() {
    auto left = parse_add_expr();
    if (!left) return nullptr;
    
    while (match(TokenType::LT) || match(TokenType::GT) || match(TokenType::LE) || 
           match(TokenType::GE) || match(TokenType::EQ) || match(TokenType::NE)) {
        BinaryExpr::Operator op = token_to_binary_op(current_token().type);
        advance_token();
        auto right = parse_add_expr();
        if (!right) return nullptr;
        
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parse_add_expr() {
    auto left = parse_mul_expr();
    if (!left) return nullptr;
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        BinaryExpr::Operator op = token_to_binary_op(current_token().type);
        advance_token();
        auto right = parse_mul_expr();
        if (!right) return nullptr;
        
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parse_mul_expr() {
    auto left = parse_unary_expr();
    if (!left) return nullptr;
    
    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE) || match(TokenType::MODULO)) {
        BinaryExpr::Operator op = token_to_binary_op(current_token().type);
        advance_token();
        auto right = parse_unary_expr();
        if (!right) return nullptr;
        
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parse_unary_expr() {
    if (match(TokenType::PLUS) || match(TokenType::MINUS) || match(TokenType::NOT)) {
        UnaryExpr::Operator op = token_to_unary_op(current_token().type);
        advance_token();
        auto operand = parse_unary_expr();
        if (!operand) return nullptr;
        
        return std::make_unique<UnaryExpr>(op, std::move(operand));
    }
    
    return parse_primary_expr();
}

std::unique_ptr<Expr> Parser::parse_primary_expr() {
    // 数字
    if (match(TokenType::NUMBER)) {
        int value = std::stoi(current_token().value);
        advance_token();
        return std::make_unique<NumberExpr>(value);
    }
    
    // 标识符或函数调用
    if (match(TokenType::IDENTIFIER)) {
        std::string name = current_token().value;
        advance_token();
        
        if (match(TokenType::LPAREN)) {
            // 函数调用
            advance_token();
            auto args = parse_arg_list();
            if (!consume(TokenType::RPAREN, "Expected ')' after function arguments")) {
                return nullptr;
            }
            return std::make_unique<CallExpr>(name, std::move(args));
        } else {
            // 变量引用
            return std::make_unique<VarExpr>(name);
        }
    }
    
    // 括号表达式
    if (match(TokenType::LPAREN)) {
        advance_token();
        auto expr = parse_expr();
        if (!expr) return nullptr;
        
        if (!consume(TokenType::RPAREN, "Expected ')' after expression")) {
            return nullptr;
        }
        return expr;
    }
    
    report_error("Expected expression");
    return nullptr;
}

std::vector<std::unique_ptr<Expr>> Parser::parse_arg_list() {
    std::vector<std::unique_ptr<Expr>> args;
    
    if (match(TokenType::RPAREN)) {
        // 空参数列表
        return args;
    }
    
    // 第一个参数
    auto arg = parse_expr();
    if (arg) {
        args.push_back(std::move(arg));
    }
    
    // 后续参数
    while (match(TokenType::COMMA)) {
        advance_token();
        auto arg = parse_expr();
        if (arg) {
            args.push_back(std::move(arg));
        } else {
            break;
        }
    }
    
    return args;
}

BinaryExpr::Operator Parser::token_to_binary_op(TokenType type) {
    switch (type) {
        case TokenType::PLUS: return BinaryExpr::Operator::ADD;
        case TokenType::MINUS: return BinaryExpr::Operator::SUB;
        case TokenType::MULTIPLY: return BinaryExpr::Operator::MUL;
        case TokenType::DIVIDE: return BinaryExpr::Operator::DIV;
        case TokenType::MODULO: return BinaryExpr::Operator::MOD;
        case TokenType::LT: return BinaryExpr::Operator::LT;
        case TokenType::GT: return BinaryExpr::Operator::GT;
        case TokenType::LE: return BinaryExpr::Operator::LE;
        case TokenType::GE: return BinaryExpr::Operator::GE;
        case TokenType::EQ: return BinaryExpr::Operator::EQ;
        case TokenType::NE: return BinaryExpr::Operator::NE;
        case TokenType::AND: return BinaryExpr::Operator::AND;
        case TokenType::OR: return BinaryExpr::Operator::OR;
        default: return BinaryExpr::Operator::ADD; 
    }
}

UnaryExpr::Operator Parser::token_to_unary_op(TokenType type) {
    switch (type) {
        case TokenType::PLUS: return UnaryExpr::Operator::PLUS;
        case TokenType::MINUS: return UnaryExpr::Operator::MINUS;
        case TokenType::NOT: return UnaryExpr::Operator::NOT;
        default: return UnaryExpr::Operator::PLUS; 
    }
}

FuncDef::ReturnType Parser::token_to_return_type(TokenType type) {
    switch (type) {
        case TokenType::INT: return FuncDef::ReturnType::INT;
        case TokenType::VOID: return FuncDef::ReturnType::VOID;
        default: return FuncDef::ReturnType::VOID; 
    }
}
