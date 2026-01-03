#include "semantic.h"
#include <iostream>
#include <sstream>

SemanticAnalyzer* analyzeHelper::semanticOwner = nullptr;

void analyzeHelper::setSemanticOwner(SemanticAnalyzer& analyzer) {
    semanticOwner = &analyzer;
}

void analyzeHelper::enterScope() {
    owner.getSymbolTables().push_back(std::unordered_map<std::string, Symbol>());
}

void analyzeHelper::exitScope() {
    if (!owner.getSymbolTables().empty()) {
        checkUnusedVariables();
        owner.getSymbolTables().pop_back();
    }
}

bool analyzeHelper::declareSymbol(const std::string &name, Symbol symbol) {
    if (owner.getSymbolTables().back().find(name) != owner.getSymbolTables().back().end()) {
        return false;
    }
    owner.getSymbolTables().back()[name] = symbol;
    return true;
}

Symbol *analyzeHelper::findSymbol(const std::string &name) {
    for (auto tableIt = owner.getSymbolTables().rbegin(); tableIt != owner.getSymbolTables().rend(); ++tableIt) {
        auto symIt = tableIt->find(name);
        if (symIt != tableIt->end()) {
            if (symIt->second.kind == Symbol::Kind::VARIABLE) {
                symIt->second.used = true;
            }
            return &symIt->second;
        }
    }
    return nullptr;
}

OptionalInt analyzeHelper::evaluateConstant(const std::shared_ptr<Expr>& expr) {
    if (auto numExpr = std::dynamic_pointer_cast<NumberExpr>(expr)) {
        return OptionalInt(numExpr->value);
    }
    
    if (auto unaryExpr = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        OptionalInt operandValue = evaluateConstant(unaryExpr->operand);
        if (!operandValue.has_value()) return OptionalInt();
        
        if (unaryExpr->op == "+") return OptionalInt(*operandValue);
        if (unaryExpr->op == "-") return OptionalInt(-(*operandValue));
        if (unaryExpr->op == "!") return OptionalInt(!(*operandValue));
        
        return OptionalInt();
    }
    
    if (auto binaryExpr = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        OptionalInt leftValue = evaluateConstant(binaryExpr->left);
        OptionalInt rightValue = evaluateConstant(binaryExpr->right);
        
        if (!leftValue.has_value() || !rightValue.has_value()) return OptionalInt();
        
        if (binaryExpr->op == "+") return OptionalInt(*leftValue + *rightValue);
        if (binaryExpr->op == "-") return OptionalInt(*leftValue - *rightValue);
        if (binaryExpr->op == "*") return OptionalInt(*leftValue * *rightValue);
        if (binaryExpr->op == "/") {
            if (*rightValue == 0) return OptionalInt();
            return OptionalInt(*leftValue / *rightValue);
        }
        if (binaryExpr->op == "%") {
            if (*rightValue == 0) return OptionalInt();
            return OptionalInt(*leftValue % *rightValue);
        }
        if (binaryExpr->op == "<") return OptionalInt(*leftValue < *rightValue ? 1 : 0);
        if (binaryExpr->op == ">") return OptionalInt(*leftValue > *rightValue ? 1 : 0);
        if (binaryExpr->op == "<=") return OptionalInt(*leftValue <= *rightValue ? 1 : 0);
        if (binaryExpr->op == ">=") return OptionalInt(*leftValue >= *rightValue ? 1 : 0);
        if (binaryExpr->op == "==") return OptionalInt(*leftValue == *rightValue ? 1 : 0);
        if (binaryExpr->op == "!=") return OptionalInt(*leftValue != *rightValue ? 1 : 0);
        if (binaryExpr->op == "&&") return OptionalInt((*leftValue && *rightValue) ? 1 : 0);
        if (binaryExpr->op == "||") return OptionalInt((*leftValue || *rightValue) ? 1 : 0);
    }
    
    return OptionalInt();
}

void analyzeHelper::error(const std::string &message, int line, int column) {
    owner.success = false;
    
    std::string fullMessage = message;
    if (line > 0) {
        fullMessage += " at line " + std::to_string(line);
        if (column > 0) {
            fullMessage += ", column " + std::to_string(column);
        }
    }

    if (reportedErrors.find(fullMessage) == reportedErrors.end()) {
        reportedErrors.insert(fullMessage);
        owner.errorMessages.push_back(fullMessage);
        
        if (semanticOwner) {
            semanticOwner->success = false;
            semanticOwner->errorMessages.push_back(fullMessage);
        }
    }
}

void analyzeHelper::warning(const std::string &message, int line, int column) {
    std::string fullMessage = message;
    if (line > 0) {
        fullMessage += " at line " + std::to_string(line);
        if (column > 0) {
            fullMessage += ", column " + std::to_string(column);
        }
    }
    
    if (reportedWarnings.find(fullMessage) == reportedWarnings.end()) {
        reportedWarnings.insert(fullMessage);
        owner.warningMessages.push_back(fullMessage);
        
        if (semanticOwner) {
            semanticOwner->warningMessages.push_back(fullMessage);
        }
    }
}

bool analyzeHelper::isValidMainFunction(FunctionDef &funcDef) {
    if (funcDef.returnType != "int") {
        error("main function must return int", funcDef.line, funcDef.column);
        return false;
    }
    
    if (!funcDef.params.empty()) {
        error("main function cannot have parameters", funcDef.line, funcDef.column);
        return false;
    }
    
    return true;
}

void analyzeHelper::checkUnusedVariables() {
    if (!owner.getSymbolTables().empty()) {
        auto& currentScope = owner.getSymbolTables().back();
        for (const auto& [name, symbol] : currentScope) {
            if (symbol.kind == Symbol::Kind::VARIABLE && !symbol.used) {
                warning("Variable '" + name + "' declared but never used", symbol.line, symbol.column);
            }
        }
    }
}

void analyzeHelper::detectDeadCode(const std::shared_ptr<Stmt>& stmt) {
    if (auto ifStmt = dynamic_cast<IfStmt*>(stmt.get())) {
        if (auto constValue = evaluateConstant(ifStmt->condition)) {
            if (*constValue) {
                if (ifStmt->elseBranch) {
                    warning("This else branch will never execute (condition always true)", 
                            getLineNumber(ifStmt->elseBranch), 
                            ifStmt->elseBranch->column);
                }
            } else {
                warning("This if branch will never execute (condition always false)", 
                        getLineNumber(ifStmt->thenBranch), 
                        ifStmt->thenBranch->column);
            }
        }
    }
    
    if (auto whileStmt = dynamic_cast<WhileStmt*>(stmt.get())) {
        if (auto constValue = evaluateConstant(whileStmt->condition)) {
            if (!(*constValue)) {
                warning("This while loop will never execute (condition always false)", 
                        whileStmt->line, whileStmt->column);
            }
        }
    }
}

bool analyzeHelper::validateFunctionCall(const std::string& name, const std::vector<std::shared_ptr<Expr>>& args, int line, int column) {
    Symbol* symbol = findSymbol(name);
    if (!symbol) {
        error("Call to undeclared function '" + name + "'", line, column);
        return false;
    }
    
    if (symbol->kind != Symbol::Kind::FUNCTION) {
        error("'" + name + "' is not a function", line, column);
        return false;
    }
    
    if (symbol->params.size() != args.size()) {
        error("Function '" + name + "' expects " + std::to_string(symbol->params.size()) + 
              " arguments but got " + std::to_string(args.size()), line, column);
        return false;
    }
    
    symbol->used = true;
    return true;
}

bool analyzeHelper::checkTypeCompatibility(const std::shared_ptr<Expr>& expr, const std::string& expectedType, int line, int column) {
    if (expectedType != "int") {
        error("Type mismatch: expected '" + expectedType + "' type", line, column);
        return false;
    }
    return true;
}

bool typeVisitor::isTypeCompatible(const std::string& sourceType, const std::string& targetType) {
    return sourceType == targetType;
}

void typeVisitor::visit(NumberExpr &) { 
    type = "int"; 
}

void typeVisitor::visit(VariableExpr &expr) {
    Symbol *symbol = owner.helper.findSymbol(expr.name);
    if (!symbol) {
        type = "error";
        return;
    }
    
    symbol->used = true;
    type = symbol->type;
}

void typeVisitor::visit(BinaryExpr &expr) {
    expr.left->accept(*this);
    std::string leftType = type;
    expr.right->accept(*this);
    std::string rightType = type;
    
    if (leftType != "int" || rightType != "int") {
        owner.helper.error("Binary operator '" + expr.op + "' requires integer operands", expr.line, expr.column);
        type = "error";
    } else {
        type = "int";
    }
}

void typeVisitor::visit(UnaryExpr &expr) {
    expr.operand->accept(*this);
    if (type != "int") {
        owner.helper.error("Unary operator '" + expr.op + "' requires integer operand", expr.line, expr.column);
        type = "error";
    } else {
        type = "int";
    }
}

void typeVisitor::visit(CallExpr &expr) {
    auto it = owner.getFunctionTable().find(expr.callee);
    if (it == owner.getFunctionTable().end()) {
        type = "error";
        return;
    }
    
    Symbol *symbol = owner.helper.findSymbol(expr.callee);
    if (symbol && symbol->kind == Symbol::Kind::FUNCTION) {
        symbol->used = true;
    }
    
    if (expr.arguments.size() != it->second.paramTypes.size()) {
        owner.helper.error("Incorrect number of arguments for function '" + expr.callee + "'", expr.line, expr.column);
        type = it->second.returnType;
        return;
    }
    
    for (size_t i = 0; i < expr.arguments.size(); ++i) {
        expr.arguments[i]->accept(*this);
        std::string argType = type;
        
        if (!isTypeCompatible(argType, it->second.paramTypes[i])) {
            owner.helper.error("Function '" + expr.callee + "' argument " + std::to_string(i+1) + 
                           " type mismatch", expr.line, expr.column);
        }
    }
    
    type = it->second.returnType;
}

void typeVisitor::visit(ExprStmt &stmt) {
    if (stmt.expression) stmt.expression->accept(*this);
}

void typeVisitor::visit(VarDeclStmt &stmt) {
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
        if (type != "int") {
            owner.helper.error("Cannot initialize integer variable with non-integer expression", stmt.line, stmt.column);
        }
    }
    type = "void";
}

void typeVisitor::visit(AssignStmt &stmt) {
    Symbol *symbol = owner.helper.findSymbol(stmt.name);
    if (!symbol) {
        type = "error";
        return;
    }
    
    symbol->used = true;
    stmt.value->accept(*this);
    std::string valueType = type;
    
    if (!isTypeCompatible(valueType, symbol->type)) {
        owner.helper.error("Assignment type mismatch: variable '" + stmt.name + "' has type '" + 
                       symbol->type + "', expression has type '" + valueType + "'", 
                       stmt.line, stmt.column);
    }
    type = "void";
}

void typeVisitor::visit(BlockStmt &stmt) {
    for (auto &s : stmt.statements) s->accept(*this);
    type = "void";
}

void typeVisitor::visit(IfStmt &stmt) {
    stmt.condition->accept(*this);
    if (type != "int") {
        owner.helper.error("If condition must be integer (used as boolean)", stmt.line, stmt.column);
    }
    
    stmt.thenBranch->accept(*this);
    if (stmt.elseBranch) stmt.elseBranch->accept(*this);
    type = "void";
}

void typeVisitor::visit(WhileStmt &stmt) {
    stmt.condition->accept(*this);
    if (type != "int") {
        owner.helper.error("While condition must be integer (used as boolean)", stmt.line, stmt.column);
    }
    
    stmt.body->accept(*this);
    type = "void";
}

void typeVisitor::visit(BreakStmt &) { 
    type = "void"; 
}

void typeVisitor::visit(ContinueStmt &) { 
    type = "void"; 
}

void typeVisitor::visit(ReturnStmt &stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
    } else {
        type = "void";
    }
}

void typeVisitor::visit(FunctionDef &funcDef) {
    funcDef.body->accept(*this);
    type = "void";
}

void typeVisitor::visit(CompUnit &compUnit) {
    for (auto &func : compUnit.functions) func->accept(*this);
    type = "void";
}

analyzeVisitor::analyzeVisitor() : typeChecker(*this), helper(*this) {
    helper.enterScope();
}

analyzeVisitor::~analyzeVisitor() {
    while (!symbolTables.empty()) {
        helper.exitScope();
    }
}

void analyzeVisitor::visit(NumberExpr &) {}

void analyzeVisitor::visit(VariableExpr &expr) {
    Symbol *symbol = helper.findSymbol(expr.name);
    if (!symbol) {
        helper.error("Undefined variable: " + expr.name, expr.line, expr.column);
        return;
    }
    symbol->used = true;
}

void analyzeVisitor::visit(BinaryExpr &expr) {
    expr.left->accept(*this);
    expr.right->accept(*this);
    
    std::string leftType = typeChecker.getExprType(*expr.left);
    std::string rightType = typeChecker.getExprType(*expr.right);
    
    if (leftType != "int" || rightType != "int") {
        helper.error("Binary operator '" + expr.op + "' requires int operands", expr.line, expr.column);
    }
    
    if (expr.op == "/" || expr.op == "%") {
        if (auto rval = helper.evaluateConstant(expr.right)) {
            if (*rval == 0) {
                helper.error("Division by zero", expr.line, expr.column);
            }
        }
    }
    
    if (expr.op == "==" || expr.op == "!=" || expr.op == "<" || expr.op == ">" || 
        expr.op == "<=" || expr.op == ">=" || expr.op == "&&" || expr.op == "||") {
        auto leftVal = helper.evaluateConstant(expr.left);
        auto rightVal = helper.evaluateConstant(expr.right);
        
        if (leftVal && rightVal) {
            bool isAlwaysTrue = false;
            
            if (expr.op == "==") isAlwaysTrue = (*leftVal == *rightVal);
            else if (expr.op == "!=") isAlwaysTrue = (*leftVal != *rightVal);
            else if (expr.op == "<") isAlwaysTrue = (*leftVal < *rightVal);
            else if (expr.op == ">") isAlwaysTrue = (*leftVal > *rightVal);
            else if (expr.op == "<=") isAlwaysTrue = (*leftVal <= *rightVal);
            else if (expr.op == ">=") isAlwaysTrue = (*leftVal >= *rightVal);
            else if (expr.op == "&&") isAlwaysTrue = (*leftVal && *rightVal);
            else if (expr.op == "||") isAlwaysTrue = (*leftVal || *rightVal);
            
            if (isAlwaysTrue) {
                helper.warning("Condition expression is always true", expr.line, expr.column);
            } else {
                helper.warning("Condition expression is always false", expr.line, expr.column);
            }
        }
    }
}

void analyzeVisitor::visit(UnaryExpr &expr) {
    expr.operand->accept(*this);
    std::string operandType = typeChecker.getExprType(*expr.operand);
    
    if (operandType != "int") {
        helper.error("Unary operator '" + expr.op + "' requires int operand", expr.line, expr.column);
    }
}

void analyzeVisitor::visit(CallExpr &expr) {
    std::string callee = expr.callee;
    if (functionTable.find(callee) == functionTable.end() && callee != currentFunction) {
        helper.error("Undefined function: " + expr.callee, expr.line, expr.column);
        return;
    }

    Symbol *symbol = helper.findSymbol(callee);
    if (symbol && symbol->kind == Symbol::Kind::FUNCTION) {
        symbol->used = true;
    }

    FunctionInfo *funcInfo = &functionTable.find(callee)->second;
    
    if (funcInfo->paramTypes.size() != expr.arguments.size()) {
        helper.error("Incorrect number of arguments for function '" + expr.callee + "'", expr.line, expr.column);
    }
    
    for (size_t i = 0; i < expr.arguments.size(); i++) {
        expr.arguments[i]->accept(*this);
        
        if (i < funcInfo->paramTypes.size()) {
            std::string argType = typeChecker.getExprType(*expr.arguments[i]);
            if (argType != funcInfo->paramTypes[i]) {
                helper.error("Function '" + expr.callee + "' argument " + std::to_string(i+1) + 
                          " type mismatch, expected '" + funcInfo->paramTypes[i] + 
                          "', got '" + argType + "'", expr.line, expr.column);
            }
        }
    }
    
    if (typeChecker.getExprType(expr) != funcInfo->returnType) {
        helper.error("Function '" + expr.callee + "' return type mismatch", expr.line, expr.column);
    }
}

void analyzeVisitor::visit(ExprStmt &stmt) {
    if (stmt.expression) stmt.expression->accept(*this);
}

void analyzeVisitor::visit(VarDeclStmt &stmt) {
    std::string name = stmt.name;
    if (symbolTables.back().find(name) != symbolTables.back().end()) {
        helper.error("Variable '" + stmt.name + "' already declared in current scope", stmt.line, stmt.column);
    }
    
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
        std::string initType = typeChecker.getExprType(*stmt.initializer);
        if (initType != "int") {
            helper.error("Cannot initialize int variable with non-integer expression", stmt.line, stmt.column);
        }
    }
    
    Symbol symbol(Symbol::Kind::VARIABLE, "int", stmt.line, stmt.column);
    symbol.used = false;
    helper.declareSymbol(stmt.name, symbol);
}

void analyzeVisitor::visit(AssignStmt &stmt) {
    Symbol *symbol = helper.findSymbol(stmt.name);
    if (!symbol) {
        helper.error("Undefined variable: " + stmt.name, stmt.line, stmt.column);
        return;
    }
    
    symbol->used = true;
    
    if (symbol->kind != Symbol::Kind::VARIABLE && symbol->kind != Symbol::Kind::PARAMETER) {
        helper.error("Cannot assign to '" + stmt.name + "' (not a variable)", stmt.line, stmt.column);
    }
    
    stmt.value->accept(*this);
    std::string valueType = typeChecker.getExprType(*stmt.value);
    
    if (valueType != "int") {
        helper.error("Type mismatch in assignment to '" + stmt.name + "'", stmt.line, stmt.column);
    }
}

void analyzeVisitor::visit(BlockStmt &stmt) {
    helper.enterScope();
    for (auto &s : stmt.statements) s->accept(*this);
    helper.exitScope();
}

void analyzeVisitor::visit(IfStmt &stmt) {
    stmt.condition->accept(*this);
    std::string condType = typeChecker.getExprType(*stmt.condition);
    
    if (condType != "int") {
        helper.error("If condition must be integer (used as boolean)", stmt.line, stmt.column);
    }
    
    auto condValue = helper.evaluateConstant(stmt.condition);
    if (condValue) {
        if (*condValue != 0) {
            if (stmt.elseBranch) {
                helper.warning("This else branch will never execute (condition always true)", 
                             stmt.elseBranch->line, stmt.elseBranch->column);
            }
        } else {
            helper.warning("This if branch will never execute (condition always false)", 
                         stmt.thenBranch->line, stmt.thenBranch->column);
        }
    }
    
    stmt.thenBranch->accept(*this);
    if (stmt.elseBranch) stmt.elseBranch->accept(*this);
}

void analyzeVisitor::visit(WhileStmt &stmt) {
    stmt.condition->accept(*this);
    std::string condType = typeChecker.getExprType(*stmt.condition);
    
    if (condType != "int") {
        helper.error("While condition must be integer (used as boolean)", stmt.line, stmt.column);
    }
    
    auto condValue = helper.evaluateConstant(stmt.condition);
    if (condValue && *condValue == 0) {
        helper.warning("This while loop will never execute (condition always false)", stmt.line, stmt.column);
    }
    
    helper.enterLoop();
    stmt.body->accept(*this);
    helper.exitLoop();
}

void analyzeVisitor::visit(BreakStmt &stmt) {
    if (!helper.isInLoop()) {
        helper.error("Break statement must be inside loop", stmt.line, stmt.column);
    }
}

void analyzeVisitor::visit(ContinueStmt &stmt) {
    if (!helper.isInLoop()) {
        helper.error("Continue statement must be inside loop", stmt.line, stmt.column);
    }
}

void analyzeVisitor::visit(ReturnStmt &stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
        std::string returnType = typeChecker.getExprType(*stmt.value);
        if (returnType != currentFunctionReturnType) {
            helper.error("Return type mismatch: expected '" + currentFunctionReturnType +
                          "', got '" + returnType + "'", stmt.line, stmt.column);
        }
    } else if (currentFunctionReturnType != "void") {
        helper.error("Function with return type '" + currentFunctionReturnType +
                      "' must return a value", stmt.line, stmt.column);
    }
    hasReturn = true;
}

void analyzeVisitor::visit(FunctionDef &funcDef) {
    int line = funcDef.line;
    int column = funcDef.column;
    std::string name = funcDef.name;

    if (functionTable.count(name)) {
        helper.error("Duplicate function name", line, column);
    }

    FunctionInfo info;
    info.returnType = funcDef.returnType;
    info.line = line;
    info.column = column;
    
    for (const auto &param : funcDef.params) {
        info.paramTypes.push_back("int");
        info.paramNames.push_back(param.name);
    }

    functionTable[name] = info;

    if (name == "main" && !helper.isValidMainFunction(funcDef)) {
        helper.error("Invalid main function declaration", line, column);
    }

    currentFunction = name;
    currentFunctionReturnType = funcDef.returnType;
    hasReturn = false;

    helper.enterScope();

    Symbol funcSymbol(Symbol::Kind::FUNCTION, funcDef.returnType, line, column);
    funcSymbol.used = (name == "main");
    helper.declareSymbol(name, funcSymbol);

    for (size_t i = 0; i < funcDef.params.size(); i++) {
        const auto &param = funcDef.params[i];
        Symbol paramSymbol(Symbol::Kind::PARAMETER, "int", line, column, i);
        paramSymbol.used = false;
        if (!helper.declareSymbol(param.name, paramSymbol)) {
            helper.error("Parameter '" + param.name + "' already declared", line, column);
        }
    }

    funcDef.body->accept(*this);

    if (funcDef.returnType != "void" && !hasReturn) {
        helper.error("Function '" + name + "' has no return statement", line, column);
    }

    checkUnusedVariables();
    helper.exitScope();

    currentFunction = "";
    currentFunctionReturnType = "";
    hasReturn = false;
}

void analyzeVisitor::visit(CompUnit &compUnit) {
    bool hasMain = false;
    for (const auto &func : compUnit.functions) {
        if (func->name == "main") {
            hasMain = true;
        }
    }
    
    if (!hasMain) {
        helper.error("Program must have a main function");
    }
    
    for (const auto &func : compUnit.functions) {
        func->accept(*this);
    }
    
    detectDeadCode();
}

void analyzeVisitor::checkUnusedVariables() {
    for (size_t scopeIndex = 0; scopeIndex < symbolTables.size(); ++scopeIndex) {
        const auto& scope = symbolTables[scopeIndex];
        for (const auto& [name, symbol] : scope) {
            if ((symbol.kind == Symbol::Kind::VARIABLE || symbol.kind == Symbol::Kind::PARAMETER) && !symbol.used) {
                helper.warning("Variable '" + name + "' declared but never used", symbol.line, symbol.column);
            }
        }
    }
}

void analyzeVisitor::detectDeadCode() {
    for (const auto& [name, info] : functionTable) {
        if (name != "main") {
            bool used = false;
            for (const auto& scope : symbolTables) {
                auto it = scope.find(name);
                if (it != scope.end() && it->second.kind == Symbol::Kind::FUNCTION && it->second.used) {
                    used = true;
                    break;
                }
            }
            if (!used) {
                helper.warning("Function '" + name + "' defined but never used", info.line);
            }
        }
    }
}

bool SemanticAnalyzer::analyze(std::shared_ptr<CompUnit> ast) {
    clearMessages();
    analyzeHelper::setSemanticOwner(*this);
    ast->accept(visitor);
    
    if (success) {
        checkUnusedVariables();
        detectDeadCode();
    }
    
    for (const auto& error : errorMessages) {
        std::cerr << "Semantic error: " << error << std::endl;
    }
    
    for (const auto& warning : warningMessages) {
        std::cerr << "Warning: " << warning << std::endl;
    }
    
    return success;
}

void SemanticAnalyzer::checkUnusedVariables() {
    visitor.checkUnusedVariables();
}

void SemanticAnalyzer::detectDeadCode() {
    visitor.detectDeadCode();
}