#pragma once

#include "ast.h"
#include "symbol_table.h"
#include <string>
#include <vector>
#include <stack>
#include <sstream>

// RISC-V32寄存器定义
namespace RISCV {
    // 通用寄存器
    constexpr const char* ZERO = "x0";   // 硬编码为0
    constexpr const char* RA = "x1";     // 返回地址
    constexpr const char* SP = "x2";     // 栈指针
    constexpr const char* GP = "x3";     // 全局指针
    constexpr const char* TP = "x4";     // 线程指针
    constexpr const char* T0 = "x5";     // 临时寄存器0
    constexpr const char* T1 = "x6";     // 临时寄存器1
    constexpr const char* T2 = "x7";     // 临时寄存器2
    constexpr const char* S0 = "x8";     // 保存寄存器0/帧指针
    constexpr const char* S1 = "x9";     // 保存寄存器1
    constexpr const char* A0 = "x10";    // 参数/返回值寄存器0
    constexpr const char* A1 = "x11";    // 参数/返回值寄存器1
    constexpr const char* A2 = "x12";    // 参数寄存器2
    constexpr const char* A3 = "x13";    // 参数寄存器3
    constexpr const char* A4 = "x14";    // 参数寄存器4
    constexpr const char* A5 = "x15";    // 参数寄存器5
    constexpr const char* A6 = "x16";    // 参数寄存器6
    constexpr const char* A7 = "x17";    // 参数寄存器7
    constexpr const char* S2 = "x18";    // 保存寄存器2
    constexpr const char* S3 = "x19";    // 保存寄存器3
    constexpr const char* S4 = "x20";    // 保存寄存器4
    constexpr const char* S5 = "x21";    // 保存寄存器5
    constexpr const char* S6 = "x22";    // 保存寄存器6
    constexpr const char* S7 = "x23";    // 保存寄存器7
    constexpr const char* S8 = "x24";    // 保存寄存器8
    constexpr const char* S9 = "x25";    // 保存寄存器9
    constexpr const char* S10 = "x26";   // 保存寄存器10
    constexpr const char* S11 = "x27";   // 保存寄存器11
    constexpr const char* T3 = "x28";    // 临时寄存器3
    constexpr const char* T4 = "x29";    // 临时寄存器4
    constexpr const char* T5 = "x30";    // 临时寄存器5
    constexpr const char* T6 = "x31";    // 临时寄存器6
}

// 代码生成器类
class CodeGenerator : public ASTVisitor {
public:
    explicit CodeGenerator(SymbolTable* sym_table);

    // 生成完整的汇编代码
    std::string generate(CompUnit& root);

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
    // 辅助方法
    void emit(const std::string& instruction);
    void emit_label(const std::string& label);
    std::string generate_label(const std::string& prefix = "L");

    // 栈操作
    void push_register(const std::string& reg);
    void pop_register(const std::string& reg);
    void allocate_stack(int size);
    void deallocate_stack(int size);

    // 寄存器分配
    std::string allocate_temp_register();
    void free_temp_register(const std::string& reg);
    void save_caller_saved_registers();
    void restore_caller_saved_registers();
    static bool is_temp_register(const std::string& reg);
    static bool is_callee_saved_register(const std::string& reg);

    // 表达式求值（返回存储结果的寄存器）
    std::string generate_expr(Expr& expr);

    // 变量地址计算
    std::string load_variable(const std::string& name);
    void store_variable(const std::string& name, const std::string& value_reg);

    // 条件分支代码生成
    void generate_condition(Expr& condition, const std::string& true_label,
        const std::string& false_label);

    // 短路求值
    std::string generate_short_circuit_and(BinaryExpr& expr);
    std::string generate_short_circuit_or(BinaryExpr& expr);

    // 函数调用约定
    void setup_function_prologue(const std::string& func_name, int local_vars_size);
    void setup_function_epilogue();
    void generate_function_call(CallExpr& call);

    // 优化相关（可选）
    bool is_optimization_enabled() const;
    void optimize_constant_folding(BinaryExpr& expr);

    // 工具函数
    static std::string escape_string(const std::string& str);

private:
    // 当前正在生成代码的函数信息
    std::string current_function_name;
    DataType current_function_return_type;
    int current_function_stack_size;

private:
    // 标签管理
    int next_label_id;
    std::stack<std::string> break_labels;    // break语句的目标标签
    std::stack<std::string> continue_labels; // continue语句的目标标签

private:
    // 寄存器管理
    std::vector<std::string> temp_registers;
    std::vector<bool> register_used;

private:
    std::ostringstream output;
    SymbolTable* symbol_table;
};