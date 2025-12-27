#pragma once

#include <memory>
#include <vector>
#include <string>
#include <iostream>

// 前向声明
class ASTVisitor;

// AST节点基类
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
    virtual void print(int indent = 0) const = 0;
    virtual std::unique_ptr<ASTNode> clone() const = 0;  // 返回基类指针
};

// 表达式基类
class Expr : public ASTNode {
public:
    virtual ~Expr() = default;
};

// 语句基类
class Stmt : public ASTNode {
public:
    virtual ~Stmt() = default;
};

// 编译单元（根节点）
class CompUnit : public ASTNode {
public:
    std::vector<std::unique_ptr<class FuncDef>> functions;

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// 函数定义
class FuncDef : public ASTNode {
public:
    enum ReturnType { INT, VOID };

    ReturnType return_type;
    std::string name;
    std::vector<std::string> params;  // 参数名列表
    std::unique_ptr<class Block> body;

    FuncDef(ReturnType ret_type, std::string func_name,
        std::vector<std::string> parameters,
        std::unique_ptr<Block> function_body);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// 语句块
class Block : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// 表达式语句
class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    explicit ExprStmt(std::unique_ptr<Expr> expr);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// 变量声明语句
class VarDecl : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> init_expr;

    VarDecl(std::string var_name, std::unique_ptr<Expr> init);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// 赋值语句
class AssignStmt : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> value;

    AssignStmt(std::string var_name, std::unique_ptr<Expr> val);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// If语句
class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_stmt;
    std::unique_ptr<Stmt> else_stmt;  // 可能为nullptr

    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then_part,
        std::unique_ptr<Stmt> else_part = nullptr);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// While语句
class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> loop_body);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// Break语句
class BreakStmt : public Stmt {
public:
    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// Continue语句
class ContinueStmt : public Stmt {
public:
    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// Return语句
class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value;  // 可能为nullptr（void函数）

    explicit ReturnStmt(std::unique_ptr<Expr> ret_value = nullptr);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// 二元表达式
class BinaryExpr : public Expr {
public:
    enum Operator {
        // 算术运算
        ADD, SUB, MUL, DIV, MOD,
        // 关系运算
        LT, GT, LE, GE, EQ, NE,
        // 逻辑运算
        AND, OR
    };

    Operator op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(Operator operation, std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针

    static std::string operatorToString(Operator op);
};

// 一元表达式
class UnaryExpr : public Expr {
public:
    enum Operator { PLUS, MINUS, NOT };

    Operator op;
    std::unique_ptr<Expr> operand;

    UnaryExpr(Operator operation, std::unique_ptr<Expr> expr);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针

    static std::string operatorToString(Operator op);
};

// 整数字面量
class NumberExpr : public Expr {
public:
    int value;

    explicit NumberExpr(int val);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// 变量引用
class VarExpr : public Expr {
public:
    std::string name;

    explicit VarExpr(std::string var_name);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// 函数调用
class CallExpr : public Expr {
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;

    CallExpr(std::string func_name, std::vector<std::unique_ptr<Expr>> arguments);

    void accept(ASTVisitor& visitor) override;
    void print(int indent = 0) const override;
    std::unique_ptr<ASTNode> clone() const override;  // 统一返回基类指针
};

// 访问者模式接口
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(CompUnit& node) = 0;
    virtual void visit(FuncDef& node) = 0;
    virtual void visit(Block& node) = 0;
    virtual void visit(ExprStmt& node) = 0;
    virtual void visit(VarDecl& node) = 0;
    virtual void visit(AssignStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(BreakStmt& node) = 0;
    virtual void visit(ContinueStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;
    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(NumberExpr& node) = 0;
    virtual void visit(VarExpr& node) = 0;
    virtual void visit(CallExpr& node) = 0;
};