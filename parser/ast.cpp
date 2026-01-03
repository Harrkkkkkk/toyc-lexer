#include "ast.h"

void NumberExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void VariableExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BinaryExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void UnaryExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CallExpr::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ExprStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void VarDeclStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void AssignStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BlockStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void IfStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void WhileStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void BreakStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ContinueStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void ReturnStmt::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void FunctionDef::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void CompUnit::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}