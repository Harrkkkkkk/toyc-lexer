#pragma once

#include "ast.h"
#include "symbol_table.h"
#include <vector>
#include <string>
#include <stack>

// 语义错误类型
enum class SemanticErrorType {
    UNDEFINED_VARIABLE,
    UNDEFINED_FUNCTION,
    REDEFINED_VARIABLE,
    REDEFINED_FUNCTION,
    TYPE_MISMATCH,
    INVALID_RETURN_TYPE,
    MISSING_RETURN_STATEMENT,
    BREAK_OUTSIDE_LOOP,
    CONTINUE_OUTSIDE_LOOP,
    FUNCTION_CALL_ARGUMENT_MISMATCH,
    DIVISION_BY_ZERO,
    VOID_FUNCTION_RETURN_VALUE,
    NON_VOID_FUNCTION_NO_RETURN
};

// 语义错误信息
class SemanticError {
public:
    SemanticErrorType type;
    std::string message;
    int line;
    int column;

    SemanticError(SemanticErrorType error_type, const std::string& msg,
        int line_num = -1, int col_num = -1);

    std::string to_string() const;
};

// 语义分析器
class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer();

    // 分析入口
    bool analyze(CompUnit& root);

    // 获取错误信息
    const std::vector<SemanticError>& get_errors() const;
    bool has_errors() const;
    void print_errors() const;

    // 获取符号表（供代码生成器使用）
    SymbolTable& get_symbol_table();

    // 工具函数
    static std::string semantic_error_type_to_string(SemanticErrorType type);
    static std::string data_type_to_string(DataType type);

    // AST访问方法
    void visit(CompUnit& node) override;
    void visit(FuncDef& node) override;
    void visit(Block& node) override;
    void visit(ExprStmt& node) override;
    void visit(VarDecl& node) override;
    void visit(AssignStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(BreakStmt& node) override;
    void visit(ContinueStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(NumberExpr& node) override;
    void visit(VarExpr& node) override;
    void visit(CallExpr& node) override;

private:
    // 错误报告
    void report_error(SemanticErrorType type, const std::string& message);

    // 类型检查
    bool check_type_compatibility(DataType expected, DataType actual);
    DataType get_binary_expr_type(BinaryExpr::Operator op, DataType left, DataType right);
    DataType get_unary_expr_type(UnaryExpr::Operator op, DataType operand);

    // 表达式类型推导
    DataType analyze_expression(Expr& expr);

    // 函数分析
    void analyze_function_definition(FuncDef& func);
    void check_function_return_paths(FuncDef& func);
    bool has_return_statement(Stmt& stmt);

    // 控制流检查
    void enter_loop();
    void exit_loop();
    bool is_in_loop() const;

    // 作用域管理
    void enter_function_scope(const std::string& func_name, DataType return_type);
    void exit_function_scope();

private:
    // 上下文状态
    std::stack<bool> in_loop_stack;      // 是否在循环中
    std::string current_function_name;   // 当前函数名
    DataType current_function_return_type; // 当前函数返回类型
    bool current_function_has_return;    // 当前函数是否有return语句

    // 表达式类型推导
    DataType last_expr_type;  // 最近计算的表达式类型

private:
    SymbolTable symbol_table;
    std::vector<SemanticError> errors;
};