#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <set>
#include <stdexcept>
#include "parser/ast.h"
#include "infos.h"

class analyzeVisitor;
class SemanticAnalyzer;

class analyzeHelper {
private:
    analyzeVisitor &owner;
    static SemanticAnalyzer* semanticOwner;
    std::set<std::string> reportedErrors;
    std::set<std::string> reportedWarnings;
    int loopDepth = 0;

public:
    explicit analyzeHelper(analyzeVisitor &owner) : owner(owner) {}
    
    static void setSemanticOwner(SemanticAnalyzer& analyzer);
    
    void enterScope();
    void exitScope();
    
    bool declareSymbol(const std::string &name, Symbol symbol);
    Symbol *findSymbol(const std::string &name);
    
    void enterLoop() { loopDepth++; }
    void exitLoop() { loopDepth--; }
    bool isInLoop() const { return loopDepth > 0; }
    
    OptionalInt evaluateConstant(const std::shared_ptr<Expr>& expr);
    
    void error(const std::string &message, int line = 0, int column = 0);
    void warning(const std::string &message, int line = 0, int column = 0);
    
    bool isValidMainFunction(FunctionDef &funcDef);
    void checkUnusedVariables();
    void detectDeadCode(const std::shared_ptr<Stmt>& stmt);
    bool validateFunctionCall(const std::string& name, const std::vector<std::shared_ptr<Expr>>& args, int line, int column);
    bool checkTypeCompatibility(const std::shared_ptr<Expr>& expr, const std::string& expectedType, int line, int column);
    
    int getLineNumber(Expr &expr) { return expr.line; }
    int getLineNumber(Stmt &stmt) { return stmt.line; }
    int getLineNumber(const std::shared_ptr<Stmt> &stmt) {
        return stmt ? getLineNumber(*stmt) : 0;
    }
    
    void resetReports() {
        reportedErrors.clear();
        reportedWarnings.clear();
    }
};

class typeVisitor : public ASTVisitor {
private:
    analyzeVisitor &owner;
    
public:
    explicit typeVisitor(analyzeVisitor &owner) : owner(owner) {}
    
    std::string type;
    
    std::string getExprType(Expr &expr) {
        expr.accept(*this);
        return type;
    }
    
    bool isTypeCompatible(const std::string& sourceType, const std::string& targetType);
    
    void visit(NumberExpr &expr) override;
    void visit(VariableExpr &expr) override;
    void visit(BinaryExpr &expr) override;
    void visit(UnaryExpr &expr) override;
    void visit(CallExpr &expr) override;
    
    void visit(ExprStmt &stmt) override;
    void visit(VarDeclStmt &stmt) override;
    void visit(AssignStmt &stmt) override;
    void visit(BlockStmt &stmt) override;
    void visit(IfStmt &stmt) override;
    void visit(WhileStmt &stmt) override;
    void visit(BreakStmt &stmt) override;
    void visit(ContinueStmt &stmt) override;
    void visit(ReturnStmt &stmt) override;
    
    void visit(FunctionDef &funcDef) override;
    void visit(CompUnit &compUnit) override;
};

class analyzeVisitor : public ASTVisitor {
private:
    std::vector<std::unordered_map<std::string, Symbol>> symbolTables;
    std::unordered_map<std::string, FunctionInfo> functionTable;
    std::string currentFunction;
    std::string currentFunctionReturnType;
    bool hasReturn = false;

public:
    typeVisitor typeChecker;
    analyzeHelper helper;
    
    bool success = true;
    std::vector<std::string> errorMessages;
    std::vector<std::string> warningMessages;
    
    analyzeVisitor();
    ~analyzeVisitor();
    
    void visit(NumberExpr &expr) override;
    void visit(VariableExpr &expr) override;
    void visit(BinaryExpr &expr) override;
    void visit(UnaryExpr &expr) override;
    void visit(CallExpr &expr) override;
    
    void visit(ExprStmt &stmt) override;
    void visit(VarDeclStmt &stmt) override;
    void visit(AssignStmt &stmt) override;
    void visit(BlockStmt &stmt) override;
    void visit(IfStmt &stmt) override;
    void visit(WhileStmt &stmt) override;
    void visit(BreakStmt &stmt) override;
    void visit(ContinueStmt &stmt) override;
    void visit(ReturnStmt &stmt) override;
    
    void visit(FunctionDef &funcDef) override;
    void visit(CompUnit &compUnit) override;
    
    std::vector<std::unordered_map<std::string, Symbol>> &getSymbolTables() { return symbolTables; }
    std::unordered_map<std::string, FunctionInfo> &getFunctionTable() { return functionTable; }
    
    void checkUnusedVariables();
    void detectDeadCode();
};

class SemanticAnalyzer {
private:
    analyzeVisitor visitor;
    
public:
    SemanticAnalyzer() : visitor() {}
    
    bool success = true;
    std::vector<std::string> errorMessages;
    std::vector<std::string> warningMessages;
    
    bool analyze(std::shared_ptr<CompUnit> ast);
    const std::vector<std::string>& getErrors() const { return errorMessages; }
    const std::vector<std::string>& getWarnings() const { return warningMessages; }
    
    void checkUnusedVariables();
    void detectDeadCode();
    
    void clearMessages() {
        errorMessages.clear();
        warningMessages.clear();
        success = true;
    }
};