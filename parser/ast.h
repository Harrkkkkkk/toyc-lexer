#pragma once
#include <string>
#include <vector>
#include <memory>

// ==================== AST节点基类 ====================

class ASTNode {
public:
    int line = 0;
    int column = 0;

    virtual ~ASTNode() = default;
    virtual void accept(class ASTVisitor& visitor) = 0;

    void setLocation(int line, int column) {
        this->line = line;
        this->column = column;
    }
};

class Expr : public ASTNode {
public:
    virtual ~Expr() = default;
};

class Stmt : public ASTNode {
public:
    virtual ~Stmt() = default;
};

// ==================== 具体AST节点类 ====================

class NumberExpr : public Expr {
public:
    int value;
    
    NumberExpr(int value, int line = 0, int column = 0) : value(value) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class VariableExpr : public Expr {
public:
    std::string name;
    
    VariableExpr(const std::string& name, int line = 0, int column = 0) : name(name) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class BinaryExpr : public Expr {
public:
    std::shared_ptr<Expr> left;
    std::string op;
    std::shared_ptr<Expr> right;
    
    BinaryExpr(std::shared_ptr<Expr> left, const std::string& op, std::shared_ptr<Expr> right,
              int line = 0, int column = 0)
        : left(left), op(op), right(right) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class UnaryExpr : public Expr {
public:
    std::string op;
    std::shared_ptr<Expr> operand;
    
    UnaryExpr(const std::string& op, std::shared_ptr<Expr> operand,
             int line = 0, int column = 0)
        : op(op), operand(operand) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class CallExpr : public Expr {
public:
    std::string callee;
    std::vector<std::shared_ptr<Expr>> arguments;
    
    CallExpr(const std::string& callee, const std::vector<std::shared_ptr<Expr>>& arguments,
            int line = 0, int column = 0)
        : callee(callee), arguments(arguments) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class ExprStmt : public Stmt {
public:
    std::shared_ptr<Expr> expression;
    
    ExprStmt(std::shared_ptr<Expr> expression, int line = 0, int column = 0)
        : expression(expression) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class VarDeclStmt : public Stmt {
public:
    std::string name;
    std::shared_ptr<Expr> initializer;
    
    VarDeclStmt(const std::string& name, std::shared_ptr<Expr> initializer,
               int line = 0, int column = 0)
        : name(name), initializer(initializer) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class AssignStmt : public Stmt {
public:
    std::string name;
    std::shared_ptr<Expr> value;
    
    AssignStmt(const std::string& name, std::shared_ptr<Expr> value,
              int line = 0, int column = 0)
        : name(name), value(value) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::shared_ptr<Stmt>> statements;
    
    BlockStmt(const std::vector<std::shared_ptr<Stmt>>& statements,
             int line = 0, int column = 0)
        : statements(statements) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class IfStmt : public Stmt {
public:
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> thenBranch;
    std::shared_ptr<Stmt> elseBranch;
    
    IfStmt(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> thenBranch, std::shared_ptr<Stmt> elseBranch,
          int line = 0, int column = 0)
        : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class WhileStmt : public Stmt {
public:
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> body;
    
    WhileStmt(std::shared_ptr<Expr> condition, std::shared_ptr<Stmt> body,
             int line = 0, int column = 0)
        : condition(condition), body(body) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class BreakStmt : public Stmt {
public:
    BreakStmt(int line = 0, int column = 0) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class ContinueStmt : public Stmt {
public:
    ContinueStmt(int line = 0, int column = 0) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class ReturnStmt : public Stmt {
public:
    std::shared_ptr<Expr> value;
    
    ReturnStmt(std::shared_ptr<Expr> value, int line = 0, int column = 0)
        : value(value) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class Param {
public:
    std::string name;
    int line = 0;
    int column = 0;
    
    Param(const std::string& name, int line = 0, int column = 0)
        : name(name), line(line), column(column) {}
};

class FunctionDef : public ASTNode {
public:
    std::string returnType;
    std::string name;
    std::vector<Param> params;
    std::shared_ptr<BlockStmt> body;
    
    FunctionDef(const std::string& returnType, const std::string& name, 
               const std::vector<Param>& params, std::shared_ptr<BlockStmt> body,
               int line = 0, int column = 0)
        : returnType(returnType), name(name), params(params), body(body) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

class CompUnit : public ASTNode {
public:
    std::vector<std::shared_ptr<FunctionDef>> functions;
    
    CompUnit(const std::vector<std::shared_ptr<FunctionDef>>& functions,
            int line = 0, int column = 0)
        : functions(functions) {
        this->line = line;
        this->column = column;
    }
    void accept(ASTVisitor& visitor) override;
};

// ==================== AST访问者接口 ====================

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    virtual void visit(NumberExpr& expr) = 0;
    virtual void visit(VariableExpr& expr) = 0;
    virtual void visit(BinaryExpr& expr) = 0;
    virtual void visit(UnaryExpr& expr) = 0;
    virtual void visit(CallExpr& expr) = 0;
    
    virtual void visit(ExprStmt& stmt) = 0;
    virtual void visit(VarDeclStmt& stmt) = 0;
    virtual void visit(AssignStmt& stmt) = 0;
    virtual void visit(BlockStmt& stmt) = 0;
    virtual void visit(IfStmt& stmt) = 0;
    virtual void visit(WhileStmt& stmt) = 0;
    virtual void visit(BreakStmt& stmt) = 0;
    virtual void visit(ContinueStmt& stmt) = 0;
    virtual void visit(ReturnStmt& stmt) = 0;
    
    virtual void visit(FunctionDef& funcDef) = 0;
    virtual void visit(CompUnit& compUnit) = 0;
};