#pragma once

#include "ast.h"
#include <memory>
#include <unordered_map>
#include <set>
// 优化器类
class Optimizer : public ASTVisitor {
public:
    explicit Optimizer(bool enable_opt = false);

    // 优化入口
    void optimize(CompUnit& root);

    // 获取优化统计信息
    int get_optimizations_count() const;

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
    // 常量折叠优化
    std::unique_ptr<Expr> constant_folding(BinaryExpr& expr);
    std::unique_ptr<Expr> constant_folding(UnaryExpr& expr);

    // 表达式简化
    std::unique_ptr<Expr> simplify_expression(std::unique_ptr<Expr> expr);

    // 控制流优化
    std::unique_ptr<Stmt> optimize_if_statement(IfStmt& if_stmt);
    std::unique_ptr<Stmt> optimize_while_statement(WhileStmt& while_stmt);

    // 死代码消除
    void eliminate_dead_code(Block& block);
    bool is_unreachable_after(Stmt& stmt);

    // 常量传播
    void propagate_constants();
    std::unique_ptr<Expr> try_constant_propagation(const Expr& expr);

    // 循环不变代码外提
    void enter_loop();
    void exit_loop();
    void analyze_loop_invariant(WhileStmt& while_stmt);
    void hoist_loop_invariants(WhileStmt& while_stmt);

    // 工具函数
    bool is_constant_expr(const Expr& expr) const;
    int evaluate_constant_expr(const Expr& expr) const;
    bool has_side_effects(const Expr& expr) const;
    std::set<std::string> collect_variables(const Expr& expr) const;
    bool depends_on_loop_variables(const Expr& expr) const;

    // 优化统计
    void record_optimization(const std::string& type);

private:
    bool optimization_enabled;
    int optimizations_applied;
    // 常量传播
    std::unordered_map<std::string, std::unique_ptr<Expr>> constant_values;
    std::unordered_map<std::string, bool> is_variable_constant;

    // 循环不变代码外提
    struct LoopInfo {
        std::set<std::string> loop_variables;  // 循环内修改的变量
        std::vector<std::unique_ptr<Stmt>> hoisted_statements; // 提到循环外的语句
    };
    std::vector<LoopInfo> loop_info_stack;
};