#include "semantic.h"
#include <iostream>
#include <sstream>


SemanticError::SemanticError(SemanticErrorType error_type, const std::string& msg, 
                           int line_num, int col_num)
    : type(error_type), message(msg), line(line_num), column(col_num) {}

std::string SemanticError::to_string() const {
    std::ostringstream oss;
    oss << SemanticAnalyzer::semantic_error_type_to_string(type) << ": " << message;
    if (line >= 0) {
        oss << " (line " << line;
        if (column >= 0) {
            oss << ", column " << column;
        }
        oss << ")";
    }
    return oss.str();
}


std::string SemanticAnalyzer::semantic_error_type_to_string(SemanticErrorType type) {
    switch (type) {
        case SemanticErrorType::UNDEFINED_VARIABLE: return "Undefined variable";
        case SemanticErrorType::UNDEFINED_FUNCTION: return "Undefined function";
        case SemanticErrorType::REDEFINED_VARIABLE: return "Variable redefinition";
        case SemanticErrorType::REDEFINED_FUNCTION: return "Function redefinition";
        case SemanticErrorType::TYPE_MISMATCH: return "Type mismatch";
        case SemanticErrorType::INVALID_RETURN_TYPE: return "Invalid return type";
        case SemanticErrorType::MISSING_RETURN_STATEMENT: return "Missing return statement";
        case SemanticErrorType::BREAK_OUTSIDE_LOOP: return "Break outside loop";
        case SemanticErrorType::CONTINUE_OUTSIDE_LOOP: return "Continue outside loop";
        case SemanticErrorType::FUNCTION_CALL_ARGUMENT_MISMATCH: return "Function call argument mismatch";
        case SemanticErrorType::DIVISION_BY_ZERO: return "Division by zero";
        case SemanticErrorType::VOID_FUNCTION_RETURN_VALUE: return "Void function returns value";
        case SemanticErrorType::NON_VOID_FUNCTION_NO_RETURN: return "Non-void function missing return";
        default: return "Unknown semantic error";
    }
}

std::string SemanticAnalyzer::data_type_to_string(DataType type) {
    switch (type) {
        case DataType::INT: return "int";
        case DataType::VOID: return "void";
        default: return "unknown";
    }
}

// SemanticAnalyzer 构造函数
SemanticAnalyzer::SemanticAnalyzer() 
    : in_loop_stack(), current_function_name(""), 
      current_function_return_type(DataType::VOID),
      current_function_has_return(false), last_expr_type(DataType::INT) {}

// SemanticAnalyzer 分析入口
bool SemanticAnalyzer::analyze(CompUnit& root) {
    errors.clear();
    
    try {
        // 分析AST
        root.accept(*this);
        
        // 检查是否有main函数
        Symbol* main_func = symbol_table.lookup_function("main");
        if (!main_func) {
            report_error(SemanticErrorType::UNDEFINED_FUNCTION, 
                        "Program must have a 'main' function");
        } else if (main_func->data_type != DataType::INT || !main_func->param_types.empty()) {
            report_error(SemanticErrorType::INVALID_RETURN_TYPE,
                        "Main function must be 'int main()' with no parameters");
        }
        
        return errors.empty();
    } catch (const std::exception& e) {
        report_error(SemanticErrorType::TYPE_MISMATCH, 
                    "Internal error during semantic analysis: " + std::string(e.what()));
        return false;
    }
}

// SemanticAnalyzer 错误信息管理
const std::vector<SemanticError>& SemanticAnalyzer::get_errors() const {
    return errors;
}

bool SemanticAnalyzer::has_errors() const {
    return !errors.empty();
}

void SemanticAnalyzer::print_errors() const {
    for (const auto& error : errors) {
        std::cerr << error.to_string() << std::endl;
    }
}

// SemanticAnalyzer 符号表访问
SymbolTable& SemanticAnalyzer::get_symbol_table() {
    return symbol_table;
}

// SemanticAnalyzer AST访问方法
void SemanticAnalyzer::visit(CompUnit& node) {
    // 第一遍：收集所有函数定义
    for (const auto& func : node.functions) {
        std::vector<DataType> param_types;
        for (size_t i = 0; i < func->params.size(); i++) {
            param_types.push_back(DataType::INT);  
        }
        
        DataType return_type = (func->return_type == FuncDef::ReturnType::INT) ? 
                              DataType::INT : DataType::VOID;
        
        if (!symbol_table.define_function(func->name, return_type, param_types)) {
            report_error(SemanticErrorType::REDEFINED_FUNCTION, 
                        "Function '" + func->name + "' is already defined");
        }
    }
    
    // 第二遍：分析函数体
    for (const auto& func : node.functions) {
        func->accept(*this);
    }
}

void SemanticAnalyzer::visit(FuncDef& node) {
    // 设置当前函数上下文
    DataType return_type = (node.return_type == FuncDef::ReturnType::INT) ? 
                          DataType::INT : DataType::VOID;
    enter_function_scope(node.name, return_type);
    
    // 进入函数作用域
    symbol_table.enter_scope();
    
    // 定义参数
    for (const auto& param_name : node.params) {
        if (!symbol_table.define_parameter(param_name, DataType::INT)) {
            report_error(SemanticErrorType::REDEFINED_VARIABLE, 
                        "Parameter '" + param_name + "' is already defined");
        }
    }
    
    // 分析函数体
    node.body->accept(*this);
    
    // 检查返回路径
    if (current_function_return_type == DataType::INT && !current_function_has_return) {
        if (!has_return_statement(*node.body)) {
            report_error(SemanticErrorType::MISSING_RETURN_STATEMENT,
                        "Function '" + node.name + "' must return a value");
        }
    }
    
    // 退出函数作用域
    symbol_table.exit_scope();
    exit_function_scope();
}

void SemanticAnalyzer::visit(Block& node) {
    // 创建新的作用域
    symbol_table.enter_scope();
    
    // 分析所有语句
    for (const auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    
    // 退出作用域
    symbol_table.exit_scope();
}

void SemanticAnalyzer::visit(ExprStmt& node) {
    if (node.expression) {
        analyze_expression(*node.expression);
    }
}

void SemanticAnalyzer::visit(VarDecl& node) {
    // 检查变量是否已定义
    if (!symbol_table.define_variable(node.name, DataType::INT)) {
        report_error(SemanticErrorType::REDEFINED_VARIABLE,
                    "Variable '" + node.name + "' is already defined in current scope");
        return;
    }
    
    // 分析初始化表达式
    DataType init_type = analyze_expression(*node.init_expr);
    if (!check_type_compatibility(DataType::INT, init_type)) {
        report_error(SemanticErrorType::TYPE_MISMATCH,
                    "Cannot initialize int variable '" + node.name + "' with " + 
                    data_type_to_string(init_type));
    }
}

void SemanticAnalyzer::visit(AssignStmt& node) {
    // 检查变量是否已定义
    Symbol* var = symbol_table.lookup_symbol(node.name);
    if (!var) {
        report_error(SemanticErrorType::UNDEFINED_VARIABLE,
                    "Variable '" + node.name + "' is not defined");
        return;
    }
    
    if (var->symbol_type != SymbolType::VARIABLE && var->symbol_type != SymbolType::PARAMETER) {
        report_error(SemanticErrorType::TYPE_MISMATCH,
                    "'" + node.name + "' is not a variable");
        return;
    }
    
    // 分析赋值表达式
    DataType value_type = analyze_expression(*node.value);
    if (!check_type_compatibility(var->data_type, value_type)) {
        report_error(SemanticErrorType::TYPE_MISMATCH,
                    "Cannot assign " + data_type_to_string(value_type) + 
                    " to " + data_type_to_string(var->data_type) + " variable '" + node.name + "'");
    }
}

void SemanticAnalyzer::visit(IfStmt& node) {
    // 分析条件表达式
    DataType cond_type = analyze_expression(*node.condition);
    if (cond_type != DataType::INT) {
        report_error(SemanticErrorType::TYPE_MISMATCH,
                    "If condition must be of int type");
    }
    
    // 分析then分支
    node.then_stmt->accept(*this);
    
    // 分析else分支
    if (node.else_stmt) {
        node.else_stmt->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStmt& node) {
    // 分析条件表达式
    DataType cond_type = analyze_expression(*node.condition);
    if (cond_type != DataType::INT) {
        report_error(SemanticErrorType::TYPE_MISMATCH,
                    "While condition must be of int type");
    }
    
    // 进入循环上下文
    enter_loop();
    
    // 分析循环体
    node.body->accept(*this);
    
    // 退出循环上下文
    exit_loop();
}

void SemanticAnalyzer::visit(BreakStmt& /* node */) {
    if (!is_in_loop()) {
        report_error(SemanticErrorType::BREAK_OUTSIDE_LOOP,
                    "Break statement can only be used inside a loop");
    }
}

void SemanticAnalyzer::visit(ContinueStmt& /* node */) {
    if (!is_in_loop()) {
        report_error(SemanticErrorType::CONTINUE_OUTSIDE_LOOP,
                    "Continue statement can only be used inside a loop");
    }
}

void SemanticAnalyzer::visit(ReturnStmt& node) {
    current_function_has_return = true;
    
    if (node.value) {
        // 有返回值的return
        if (current_function_return_type == DataType::VOID) {
            report_error(SemanticErrorType::VOID_FUNCTION_RETURN_VALUE,
                        "Void function cannot return a value");
        } else {
            DataType return_type = analyze_expression(*node.value);
            if (!check_type_compatibility(current_function_return_type, return_type)) {
                report_error(SemanticErrorType::INVALID_RETURN_TYPE,
                            "Return type mismatch: expected " + 
                            data_type_to_string(current_function_return_type) + 
                            ", got " + data_type_to_string(return_type));
            }
        }
    } else {
        // 无返回值的return
        if (current_function_return_type != DataType::VOID) {
            report_error(SemanticErrorType::NON_VOID_FUNCTION_NO_RETURN,
                        "Non-void function must return a value");
        }
    }
}

void SemanticAnalyzer::visit(BinaryExpr& node) {
    DataType left_type = analyze_expression(*node.left);
    DataType right_type = analyze_expression(*node.right);
    
    last_expr_type = get_binary_expr_type(node.op, left_type, right_type);
    
    // 检查除零
    if ((node.op == BinaryExpr::Operator::DIV || node.op == BinaryExpr::Operator::MOD)) {
        if (auto* num_expr = dynamic_cast<NumberExpr*>(node.right.get())) {
            if (num_expr->value == 0) {
                report_error(SemanticErrorType::DIVISION_BY_ZERO,
                            "Division by zero");
            }
        }
    }
}

void SemanticAnalyzer::visit(UnaryExpr& node) {
    DataType operand_type = analyze_expression(*node.operand);
    last_expr_type = get_unary_expr_type(node.op, operand_type);
}

void SemanticAnalyzer::visit(NumberExpr& /* node */) {
    last_expr_type = DataType::INT;
}

void SemanticAnalyzer::visit(VarExpr& node) {
    Symbol* var = symbol_table.lookup_symbol(node.name);
    if (!var) {
        report_error(SemanticErrorType::UNDEFINED_VARIABLE,
                    "Variable '" + node.name + "' is not defined");
        last_expr_type = DataType::INT;  
        return;
    }
    
    if (var->symbol_type != SymbolType::VARIABLE && var->symbol_type != SymbolType::PARAMETER) {
        report_error(SemanticErrorType::TYPE_MISMATCH,
                    "'" + node.name + "' is not a variable");
        last_expr_type = DataType::INT;
        return;
    }
    
    last_expr_type = var->data_type;
}

void SemanticAnalyzer::visit(CallExpr& node) {
    Symbol* func = symbol_table.lookup_function(node.name);
    if (!func) {
        report_error(SemanticErrorType::UNDEFINED_FUNCTION,
                    "Function '" + node.name + "' is not defined");
        last_expr_type = DataType::INT;  
        return;
    }
    
    // 检查参数数量
    if (node.args.size() != func->param_types.size()) {
        report_error(SemanticErrorType::FUNCTION_CALL_ARGUMENT_MISMATCH,
                    "Function '" + node.name + "' expects " + 
                    std::to_string(func->param_types.size()) + " arguments, got " + 
                    std::to_string(node.args.size()));
        last_expr_type = func->data_type;
        return;
    }
    
    // 检查参数类型
    for (size_t i = 0; i < node.args.size(); i++) {
        DataType arg_type = analyze_expression(*node.args[i]);
        if (!check_type_compatibility(func->param_types[i], arg_type)) {
            report_error(SemanticErrorType::FUNCTION_CALL_ARGUMENT_MISMATCH,
                        "Argument " + std::to_string(i + 1) + " type mismatch: expected " + 
                        data_type_to_string(func->param_types[i]) + ", got " + 
                        data_type_to_string(arg_type));
        }
    }
    
    last_expr_type = func->data_type;
}

// SemanticAnalyzer 私有辅助方法
void SemanticAnalyzer::report_error(SemanticErrorType type, const std::string& message) {
    errors.emplace_back(type, message);
}

// SemanticAnalyzer 类型检查
bool SemanticAnalyzer::check_type_compatibility(DataType expected, DataType actual) {
    return expected == actual;
}

DataType SemanticAnalyzer::get_binary_expr_type(BinaryExpr::Operator op, DataType left, DataType right) {
     if (left != DataType::INT || right != DataType::INT) {
        std::string op_str = BinaryExpr::operatorToString(op);
        std::string left_str = data_type_to_string(left);
        std::string right_str = data_type_to_string(right);
        
        if (left != DataType::INT && right != DataType::INT) {
            report_error(SemanticErrorType::TYPE_MISMATCH,
                        "Binary operator '" + op_str + "' cannot be applied to types '" + 
                        left_str + "' and '" + right_str + "'");
        } else if (left != DataType::INT) {
            report_error(SemanticErrorType::TYPE_MISMATCH,
                        "Left operand of '" + op_str + "' has type '" + 
                        left_str + "' but expected 'int'");
        } else {
            report_error(SemanticErrorType::TYPE_MISMATCH,
                        "Right operand of '" + op_str + "' has type '" + 
                        right_str + "' but expected 'int'");
        }
    }
    
    // ToyC中所有二元表达式都返回int类型
    return DataType::INT;
}

DataType SemanticAnalyzer::get_unary_expr_type(UnaryExpr::Operator op, DataType operand) {
     if (operand != DataType::INT) {
        std::string op_str = UnaryExpr::operatorToString(op);
        std::string operand_str = data_type_to_string(operand);
        report_error(SemanticErrorType::TYPE_MISMATCH,
                    "Unary operator '" + op_str + "' cannot be applied to type '" + 
                    operand_str + "'");
    }
    
    // ToyC中所有一元表达式都返回int类型
    return DataType::INT;
}

// SemanticAnalyzer 表达式类型推导
DataType SemanticAnalyzer::analyze_expression(Expr& expr) {
    expr.accept(*this);
    return last_expr_type;
}

// SemanticAnalyzer 函数分析
bool SemanticAnalyzer::has_return_statement(Stmt& stmt) {
    // 简单检查：递归查找return语句
    if (dynamic_cast<ReturnStmt*>(&stmt)) {
        return true;
    }
    if (auto* block = dynamic_cast<Block*>(&stmt)) {
        for (const auto& s : block->statements) {
            if (has_return_statement(*s)) {
                return true;
            }
        }
    }
    
    if (auto* if_stmt = dynamic_cast<IfStmt*>(&stmt)) {
        // 对于if语句，只有当both分支都有return时才算有return
        if (if_stmt->else_stmt) {
            return has_return_statement(*if_stmt->then_stmt) && 
                   has_return_statement(*if_stmt->else_stmt);
        }
    }
    
    return false;
}

// SemanticAnalyzer 控制流检查
void SemanticAnalyzer::enter_loop() {
    in_loop_stack.push(true);
}

void SemanticAnalyzer::exit_loop() {
    if (!in_loop_stack.empty()) {
        in_loop_stack.pop();
    }
}

bool SemanticAnalyzer::is_in_loop() const {
    return !in_loop_stack.empty() && in_loop_stack.top();
}

// SemanticAnalyzer 作用域管理
void SemanticAnalyzer::enter_function_scope(const std::string& func_name, DataType return_type) {
    current_function_name = func_name;
    current_function_return_type = return_type;
    current_function_has_return = false;
}

void SemanticAnalyzer::exit_function_scope() {
    current_function_name = "";
    current_function_return_type = DataType::VOID;
    current_function_has_return = false;
}
