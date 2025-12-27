#pragma once

#include "lexer.h"
#include "ast.h"
#include <memory>
#include <vector>

// 递归下降语法分析器
class Parser {
public:
    explicit Parser(const std::vector<Token>& token_list);

    // 解析入口
    std::unique_ptr<CompUnit> parse();

    // 错误处理
    void report_error(const std::string& message);
    bool has_error;
    std::string error_message;

private:
    // Token操作
    const Token& current_token();
    const Token& peek_token(size_t offset = 1);
    void advance_token();
    bool match(TokenType type);
    bool consume(TokenType type, const std::string& error_msg = "");

    // 语法分析方法
    std::unique_ptr<CompUnit> parse_comp_unit();
    std::unique_ptr<FuncDef> parse_func_def();
    std::vector<std::string> parse_param_list();
    std::unique_ptr<Block> parse_block();
    std::unique_ptr<Stmt> parse_stmt();

    // 表达式解析
    std::unique_ptr<Expr> parse_expr();
    std::unique_ptr<Expr> parse_lor_expr();
    std::unique_ptr<Expr> parse_land_expr();
    std::unique_ptr<Expr> parse_rel_expr();
    std::unique_ptr<Expr> parse_add_expr();
    std::unique_ptr<Expr> parse_mul_expr();
    std::unique_ptr<Expr> parse_unary_expr();
    std::unique_ptr<Expr> parse_primary_expr();

    // 函数调用参数
    std::vector<std::unique_ptr<Expr>> parse_arg_list();

    // 工具方法
    BinaryExpr::Operator token_to_binary_op(TokenType type);
    UnaryExpr::Operator token_to_unary_op(TokenType type);
    FuncDef::ReturnType token_to_return_type(TokenType type);

private:
    std::vector<Token> tokens;
    size_t current_token_index;
};