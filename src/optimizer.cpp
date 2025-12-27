#include "optimizer.h"
#include <iostream>
#include <limits>
#include <algorithm>

Optimizer::Optimizer(bool enable_opt) 
    : optimization_enabled(enable_opt), optimizations_applied(0) {}

void Optimizer::optimize(CompUnit& root) {
    if (!optimization_enabled) {
        return;
    }
    
    optimizations_applied = 0;
    constant_values.clear();
    is_variable_constant.clear();
    loop_info_stack.clear();
    
    root.accept(*this);
    
    #ifdef DEBUG_OPTIMIZATION
    std::cerr << "Applied " << optimizations_applied << " optimizations" << std::endl;
    #endif
}

int Optimizer::get_optimizations_count() const {
    return optimizations_applied;
}

void Optimizer::visit(CompUnit& node) {
    for (const auto& func : node.functions) {
        func->accept(*this);
    }
}

void Optimizer::visit(FuncDef& node) {
    node.body->accept(*this);
}

void Optimizer::visit(Block& node) {
    // 优化每个语句
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    
    // 死代码消除
    eliminate_dead_code(node);
}

void Optimizer::visit(ExprStmt& node) {
    if (node.expression) {
        node.expression = simplify_expression(std::move(node.expression));
    }
}

void Optimizer::visit(VarDecl& node) {
    // 优化初始化表达式
    node.init_expr = simplify_expression(std::move(node.init_expr));
    
    // 常量传播：如果初始化表达式是常量，记录常量值
    if (node.init_expr && is_constant_expr(*node.init_expr)) {
        // 复制表达式用于常量值存储
        if (auto* num_expr = dynamic_cast<NumberExpr*>(node.init_expr.get())) {
            // 存储为NumberExpr的克隆
            constant_values[node.name] = std::make_unique<NumberExpr>(num_expr->value);
            is_variable_constant[node.name] = true;
            record_optimization("constant propagation - variable initialization");
        }
    }
}

void Optimizer::visit(AssignStmt& node) {
    // 优化赋值表达式
    node.value = simplify_expression(std::move(node.value));
    
    // 常量传播：如果赋值是常量，更新常量值
    if (node.value && is_constant_expr(*node.value)) {
        if (auto* num_expr = dynamic_cast<NumberExpr*>(node.value.get())) {
            constant_values[node.name] = std::make_unique<NumberExpr>(num_expr->value);
            is_variable_constant[node.name] = true;
            record_optimization("constant propagation - variable assignment");
        }
    } else {
        // 变量被赋予非常量值，不再是常量
        if (is_variable_constant.count(node.name)) {
            constant_values.erase(node.name);
            is_variable_constant.erase(node.name);
        }
    }
    
    // 记录循环中修改的变量
    if (!loop_info_stack.empty()) {
        loop_info_stack.back().loop_variables.insert(node.name);
    }
}

void Optimizer::visit(IfStmt& node) {
    // 优化条件表达式
    node.condition = simplify_expression(std::move(node.condition));
    
    // 尝试优化整个if语句
    auto optimized_if = optimize_if_statement(node);
    if (optimized_if) {
        // 这里需要额外的机制来替换节点，简化实现中跳过
        record_optimization("if-statement simplification");
    }
    
    // 递归优化分支
    node.then_stmt->accept(*this);
    if (node.else_stmt) {
        node.else_stmt->accept(*this);
    }
}

void Optimizer::visit(WhileStmt& node) {
    // 进入循环上下文
    enter_loop();
    
    // 分析循环不变代码
    analyze_loop_invariant(node);
    
    // 优化条件表达式
    node.condition = simplify_expression(std::move(node.condition));
    
    // 尝试优化整个while语句
    auto optimized_while = optimize_while_statement(node);
    if (optimized_while) {
        record_optimization("while-statement simplification");
    }
    
    // 循环不变代码外提
    hoist_loop_invariants(node);
    
    // 递归优化循环体
    node.body->accept(*this);
    
    // 退出循环上下文
    exit_loop();
}

void Optimizer::visit(BreakStmt& /* node */) {
    // break语句不需要优化
}

void Optimizer::visit(ContinueStmt& /* node */) {
    // continue语句不需要优化
}

void Optimizer::visit(ReturnStmt& node) {
    if (node.value) {
        node.value = simplify_expression(std::move(node.value));
    }
}

void Optimizer::visit(BinaryExpr& node) {
    // 递归优化子表达式
    node.left->accept(*this);
    node.right->accept(*this);
    
    // 尝试常量折叠
    auto folded = constant_folding(node);
    if (folded) {
        // 这里需要额外的机制来替换节点，简化实现中跳过
        record_optimization("constant folding");
    }
}

void Optimizer::visit(UnaryExpr& node) {
    // 递归优化子表达式
    node.operand->accept(*this);
    
    // 尝试常量折叠
    auto folded = constant_folding(node);
    if (folded) {
        record_optimization("constant folding");
    }
}

void Optimizer::visit(NumberExpr& /* node */) {
    // 数字字面量不需要优化
}

void Optimizer::visit(VarExpr& node) {
    // 常量传播：如果变量有常量值，替换为常量表达式
    // 注意：实际替换需要在 simplify_expression 中处理
    // 这里只是记录，实际替换在 try_constant_propagation 中完成
    if (is_variable_constant.count(node.name) && constant_values.count(node.name)) {
        // 在 simplify_expression 中会处理这个替换
    }
}

void Optimizer::visit(CallExpr& node) {
    // 优化函数调用的参数
    for (auto& arg : node.args) {
        arg = simplify_expression(std::move(arg));
    }
}

// 私有方法实现
std::unique_ptr<Expr> Optimizer::constant_folding(BinaryExpr& expr) {
    // 检查是否两个操作数都是常量（包括常量传播后的变量）
    bool left_const = is_constant_expr(*expr.left) || 
                     (dynamic_cast<VarExpr*>(expr.left.get()) && 
                      is_variable_constant.count(dynamic_cast<VarExpr*>(expr.left.get())->name));
    
    bool right_const = is_constant_expr(*expr.right) || 
                      (dynamic_cast<VarExpr*>(expr.right.get()) && 
                       is_variable_constant.count(dynamic_cast<VarExpr*>(expr.right.get())->name));
    
    if (!left_const || !right_const) {
        return nullptr;
    }
    
    int left_val = evaluate_constant_expr(*expr.left);
    int right_val = evaluate_constant_expr(*expr.right);
    int result;
    
    switch (expr.op) {
        case BinaryExpr::Operator::ADD:
            result = left_val + right_val;
            break;
        case BinaryExpr::Operator::SUB:
            result = left_val - right_val;
            break;
        case BinaryExpr::Operator::MUL:
            result = left_val * right_val;
            break;
        case BinaryExpr::Operator::DIV:
            if (right_val == 0) return nullptr;  // 避免除零
            result = left_val / right_val;
            break;
        case BinaryExpr::Operator::MOD:
            if (right_val == 0) return nullptr;  // 避免除零
            result = left_val % right_val;
            break;
        case BinaryExpr::Operator::LT:
            result = left_val < right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::GT:
            result = left_val > right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::LE:
            result = left_val <= right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::GE:
            result = left_val >= right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::EQ:
            result = left_val == right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::NE:
            result = left_val != right_val ? 1 : 0;
            break;
        case BinaryExpr::Operator::AND:
            result = (left_val && right_val) ? 1 : 0;
            break;
        case BinaryExpr::Operator::OR:
            result = (left_val || right_val) ? 1 : 0;
            break;
        default:
            return nullptr;
    }
    
    return std::make_unique<NumberExpr>(result);
}

std::unique_ptr<Expr> Optimizer::constant_folding(UnaryExpr& expr) {
    bool operand_const = is_constant_expr(*expr.operand) || 
                        (dynamic_cast<VarExpr*>(expr.operand.get()) && 
                         is_variable_constant.count(dynamic_cast<VarExpr*>(expr.operand.get())->name));
    
    if (!operand_const) {
        return nullptr;
    }
    
    int operand_val = evaluate_constant_expr(*expr.operand);
    int result;
    
    switch (expr.op) {
        case UnaryExpr::Operator::PLUS:
            result = operand_val;
            break;
        case UnaryExpr::Operator::MINUS:
            result = -operand_val;
            break;
        case UnaryExpr::Operator::NOT:
            result = operand_val ? 0 : 1;
            break;
        default:
            return nullptr;
    }
    
    return std::make_unique<NumberExpr>(result);
}

std::unique_ptr<Expr> Optimizer::simplify_expression(std::unique_ptr<Expr> expr) {
    if (!expr) return expr;
    
    // 先尝试常量传播
    auto propagated = try_constant_propagation(*expr);
    if (propagated) {
        record_optimization("constant propagation in expression");
        expr = std::move(propagated);
    }
    
    // 然后尝试常量折叠和简化
    if (auto* bin_expr = dynamic_cast<BinaryExpr*>(expr.get())) {
        auto folded = constant_folding(*bin_expr);
        if (folded) {
            record_optimization("binary expression constant folding");
            return folded;
        }
        
        // 代数简化规则
        // x + 0 = x
        if (bin_expr->op == BinaryExpr::Operator::ADD && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 0) {
                record_optimization("x + 0 = x");
                return std::move(bin_expr->left);
            }
        }
        
        // 0 + x = x
        if (bin_expr->op == BinaryExpr::Operator::ADD && is_constant_expr(*bin_expr->left)) {
            if (evaluate_constant_expr(*bin_expr->left) == 0) {
                record_optimization("0 + x = x");
                return std::move(bin_expr->right);
            }
        }
        
        // x * 1 = x
        if (bin_expr->op == BinaryExpr::Operator::MUL && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 1) {
                record_optimization("x * 1 = x");
                return std::move(bin_expr->left);
            }
        }
        
        // 1 * x = x
        if (bin_expr->op == BinaryExpr::Operator::MUL && is_constant_expr(*bin_expr->left)) {
            if (evaluate_constant_expr(*bin_expr->left) == 1) {
                record_optimization("1 * x = x");
                return std::move(bin_expr->right);
            }
        }
        
        // x * 0 = 0
        if (bin_expr->op == BinaryExpr::Operator::MUL) {
            if ((is_constant_expr(*bin_expr->left) && evaluate_constant_expr(*bin_expr->left) == 0) ||
                (is_constant_expr(*bin_expr->right) && evaluate_constant_expr(*bin_expr->right) == 0)) {
                record_optimization("x * 0 = 0");
                return std::make_unique<NumberExpr>(0);
            }
        }
        
        // x - 0 = x
        if (bin_expr->op == BinaryExpr::Operator::SUB && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 0) {
                record_optimization("x - 0 = x");
                return std::move(bin_expr->left);
            }
        }
        
        // x - x = 0 (当没有副作用时)
        if (bin_expr->op == BinaryExpr::Operator::SUB) {
            if (auto* left_var = dynamic_cast<VarExpr*>(bin_expr->left.get())) {
                if (auto* right_var = dynamic_cast<VarExpr*>(bin_expr->right.get())) {
                    if (left_var->name == right_var->name) {
                        record_optimization("x - x = 0");
                        return std::make_unique<NumberExpr>(0);
                    }
                }
            }
        }
        
        // x / 1 = x (确保没有除零)
        if (bin_expr->op == BinaryExpr::Operator::DIV && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 1) {
                record_optimization("x / 1 = x");
                return std::move(bin_expr->left);
            }
        }
        
        // x % 1 = 0 (对于正整数)
        if (bin_expr->op == BinaryExpr::Operator::MOD && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 1) {
                record_optimization("x % 1 = 0");
                return std::make_unique<NumberExpr>(0);
            }
        }
        
        // 逻辑运算简化
        // x && false = false
        if (bin_expr->op == BinaryExpr::Operator::AND && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 0) {
                record_optimization("x && false = false");
                return std::make_unique<NumberExpr>(0);
            }
        }
        
        // x && true = x
        if (bin_expr->op == BinaryExpr::Operator::AND && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) != 0) {
                record_optimization("x && true = x");
                return std::move(bin_expr->left);
            }
        }
        
        // false && x = false
        if (bin_expr->op == BinaryExpr::Operator::AND && is_constant_expr(*bin_expr->left)) {
            if (evaluate_constant_expr(*bin_expr->left) == 0) {
                record_optimization("false && x = false");
                return std::make_unique<NumberExpr>(0);
            }
        }
        
        // true && x = x
        if (bin_expr->op == BinaryExpr::Operator::AND && is_constant_expr(*bin_expr->left)) {
            if (evaluate_constant_expr(*bin_expr->left) != 0) {
                record_optimization("true && x = x");
                return std::move(bin_expr->right);
            }
        }
        
        // x || false = x
        if (bin_expr->op == BinaryExpr::Operator::OR && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) == 0) {
                record_optimization("x || false = x");
                return std::move(bin_expr->left);
            }
        }
        
        // x || true = true
        if (bin_expr->op == BinaryExpr::Operator::OR && is_constant_expr(*bin_expr->right)) {
            if (evaluate_constant_expr(*bin_expr->right) != 0) {
                record_optimization("x || true = true");
                return std::make_unique<NumberExpr>(1);
            }
        }
        
        // false || x = x
        if (bin_expr->op == BinaryExpr::Operator::OR && is_constant_expr(*bin_expr->left)) {
            if (evaluate_constant_expr(*bin_expr->left) == 0) {
                record_optimization("false || x = x");
                return std::move(bin_expr->right);
            }
        }
        
        // true || x = true
        if (bin_expr->op == BinaryExpr::Operator::OR && is_constant_expr(*bin_expr->left)) {
            if (evaluate_constant_expr(*bin_expr->left) != 0) {
                record_optimization("true || x = true");
                return std::make_unique<NumberExpr>(1);
            }
        }
        
        // 比较运算简化
        // x == x = true (当没有副作用时)
        if (bin_expr->op == BinaryExpr::Operator::EQ) {
            if (auto* left_var = dynamic_cast<VarExpr*>(bin_expr->left.get())) {
                if (auto* right_var = dynamic_cast<VarExpr*>(bin_expr->right.get())) {
                    if (left_var->name == right_var->name) {
                        record_optimization("x == x = true");
                        return std::make_unique<NumberExpr>(1);
                    }
                }
            }
        }
        
        // x != x = false (当没有副作用时)
        if (bin_expr->op == BinaryExpr::Operator::NE) {
            if (auto* left_var = dynamic_cast<VarExpr*>(bin_expr->left.get())) {
                if (auto* right_var = dynamic_cast<VarExpr*>(bin_expr->right.get())) {
                    if (left_var->name == right_var->name) {
                        record_optimization("x != x = false");
                        return std::make_unique<NumberExpr>(0);
                    }
                }
            }
        }
        
        // x < x = false
        if (bin_expr->op == BinaryExpr::Operator::LT) {
            if (auto* left_var = dynamic_cast<VarExpr*>(bin_expr->left.get())) {
                if (auto* right_var = dynamic_cast<VarExpr*>(bin_expr->right.get())) {
                    if (left_var->name == right_var->name) {
                        record_optimization("x < x = false");
                        return std::make_unique<NumberExpr>(0);
                    }
                }
            }
        }
        
        // x <= x = true
        if (bin_expr->op == BinaryExpr::Operator::LE) {
            if (auto* left_var = dynamic_cast<VarExpr*>(bin_expr->left.get())) {
                if (auto* right_var = dynamic_cast<VarExpr*>(bin_expr->right.get())) {
                    if (left_var->name == right_var->name) {
                        record_optimization("x <= x = true");
                        return std::make_unique<NumberExpr>(1);
                    }
                }
            }
        }
        
        // x > x = false
        if (bin_expr->op == BinaryExpr::Operator::GT) {
            if (auto* left_var = dynamic_cast<VarExpr*>(bin_expr->left.get())) {
                if (auto* right_var = dynamic_cast<VarExpr*>(bin_expr->right.get())) {
                    if (left_var->name == right_var->name) {
                        record_optimization("x > x = false");
                        return std::make_unique<NumberExpr>(0);
                    }
                }
            }
        }
        
        // x >= x = true
        if (bin_expr->op == BinaryExpr::Operator::GE) {
            if (auto* left_var = dynamic_cast<VarExpr*>(bin_expr->left.get())) {
                if (auto* right_var = dynamic_cast<VarExpr*>(bin_expr->right.get())) {
                    if (left_var->name == right_var->name) {
                        record_optimization("x >= x = true");
                        return std::make_unique<NumberExpr>(1);
                    }
                }
            }
        }
        
        // 0 - x = -x (当x是常量或变量)
        if (bin_expr->op == BinaryExpr::Operator::SUB && is_constant_expr(*bin_expr->left)) {
            if (evaluate_constant_expr(*bin_expr->left) == 0) {
                record_optimization("0 - x = -x");
                return std::make_unique<UnaryExpr>(
                    UnaryExpr::Operator::MINUS,
                    std::move(bin_expr->right)
                );
            }
        }
    }
    
    if (auto* unary_expr = dynamic_cast<UnaryExpr*>(expr.get())) {
        auto folded = constant_folding(*unary_expr);
        if (folded) {
            record_optimization("unary expression constant folding");
            return folded;
        }
        
        // --x = x
        if (unary_expr->op == UnaryExpr::Operator::MINUS) {
            if (auto* inner_unary = dynamic_cast<UnaryExpr*>(unary_expr->operand.get())) {
                if (inner_unary->op == UnaryExpr::Operator::MINUS) {
                    record_optimization("--x = x");
                    return std::move(inner_unary->operand);
                }
            }
        }
        
        // !!x = (x != 0) (在ToyC中，这转换为bool值)
        if (unary_expr->op == UnaryExpr::Operator::NOT) {
            if (auto* inner_unary = dynamic_cast<UnaryExpr*>(unary_expr->operand.get())) {
                if (inner_unary->op == UnaryExpr::Operator::NOT) {
                    record_optimization("!!x simplification");
                    return std::make_unique<BinaryExpr>(
                        BinaryExpr::Operator::NE,
                        std::move(inner_unary->operand),
                        std::make_unique<NumberExpr>(0)
                    );
                }
            }
        }
        
        // !(x == y) = (x != y)
        if (unary_expr->op == UnaryExpr::Operator::NOT) {
            if (auto* inner_binary = dynamic_cast<BinaryExpr*>(unary_expr->operand.get())) {
                if (inner_binary->op == BinaryExpr::Operator::EQ) {
                    record_optimization("!(x == y) = (x != y)");
                    return std::make_unique<BinaryExpr>(
                        BinaryExpr::Operator::NE,
                        std::move(inner_binary->left),
                        std::move(inner_binary->right)
                    );
                } else if (inner_binary->op == BinaryExpr::Operator::NE) {
                    record_optimization("!(x != y) = (x == y)");
                    return std::make_unique<BinaryExpr>(
                        BinaryExpr::Operator::EQ,
                        std::move(inner_binary->left),
                        std::move(inner_binary->right)
                    );
                } else if (inner_binary->op == BinaryExpr::Operator::LT) {
                    record_optimization("!(x < y) = (x >= y)");
                    return std::make_unique<BinaryExpr>(
                        BinaryExpr::Operator::GE,
                        std::move(inner_binary->left),
                        std::move(inner_binary->right)
                    );
                } else if (inner_binary->op == BinaryExpr::Operator::GT) {
                    record_optimization("!(x > y) = (x <= y)");
                    return std::make_unique<BinaryExpr>(
                        BinaryExpr::Operator::LE,
                        std::move(inner_binary->left),
                        std::move(inner_binary->right)
                    );
                } else if (inner_binary->op == BinaryExpr::Operator::LE) {
                    record_optimization("!(x <= y) = (x > y)");
                    return std::make_unique<BinaryExpr>(
                        BinaryExpr::Operator::GT,
                        std::move(inner_binary->left),
                        std::move(inner_binary->right)
                    );
                } else if (inner_binary->op == BinaryExpr::Operator::GE) {
                    record_optimization("!(x >= y) = (x < y)");
                    return std::make_unique<BinaryExpr>(
                        BinaryExpr::Operator::LT,
                        std::move(inner_binary->left),
                        std::move(inner_binary->right)
                    );
                }
            }
        }
    }
    
    return expr;
}

void Optimizer::eliminate_dead_code(Block& block) {
    auto& statements = block.statements;
    
    for (auto it = statements.begin(); it != statements.end(); ) {
        // 检查是否为死代码（在return之后的代码）
        if (it != statements.begin()) {
            auto prev_it = it - 1;
            if (is_unreachable_after(**prev_it)) {
                record_optimization("dead code elimination");
                it = statements.erase(it);
                continue;
            }
        }
        
        // 检查表达式语句是否有副作用
        if (auto* expr_stmt = dynamic_cast<ExprStmt*>(it->get())) {
            if (expr_stmt->expression && !has_side_effects(*expr_stmt->expression)) {
                record_optimization("dead expression elimination");
                it = statements.erase(it);
                continue;
            }
        }
        
        ++it;
    }
}

bool Optimizer::is_unreachable_after(Stmt& stmt) {
    return dynamic_cast<ReturnStmt*>(&stmt) != nullptr ||
           dynamic_cast<BreakStmt*>(&stmt) != nullptr ||
           dynamic_cast<ContinueStmt*>(&stmt) != nullptr;
}

std::unique_ptr<Stmt> Optimizer::optimize_if_statement(IfStmt& if_stmt) {
    // 如果条件是常量，可以简化if语句
    if (is_constant_expr(*if_stmt.condition)) {
        int cond_val = evaluate_constant_expr(*if_stmt.condition);
        if (cond_val) {
            // 条件总是真，返回then分支
            return std::move(if_stmt.then_stmt);
        } else {
            // 条件总是假，返回else分支（如果存在）
            if (if_stmt.else_stmt) {
                return std::move(if_stmt.else_stmt);
            } else {
                // 返回空语句
                return std::make_unique<ExprStmt>(nullptr);
            }
        }
    }
    
    return nullptr;  // 无法优化
}

std::unique_ptr<Stmt> Optimizer::optimize_while_statement(WhileStmt& while_stmt) {
    // 如果条件是常量0，整个循环可以消除
    if (is_constant_expr(*while_stmt.condition)) {
        int cond_val = evaluate_constant_expr(*while_stmt.condition);
        if (cond_val == 0) {
            // 循环永远不执行
            return std::make_unique<ExprStmt>(nullptr);
        }
        // 如果条件是非零常量，保持原样（无限循环）
    }
    
    return nullptr;  // 无法优化
}

// 常量传播
std::unique_ptr<Expr> Optimizer::try_constant_propagation(const Expr& expr) {
    if (const auto* var_expr = dynamic_cast<const VarExpr*>(&expr)) {
        auto it = constant_values.find(var_expr->name);
        if (it != constant_values.end()) {
            // 克隆常量值表达式
            if (auto* num_expr = dynamic_cast<const NumberExpr*>(it->second.get())) {
                return std::make_unique<NumberExpr>(num_expr->value);
            }
        }
    }
    
    // 递归处理复合表达式
    if (const auto* bin_expr = dynamic_cast<const BinaryExpr*>(&expr)) {
        auto new_left = try_constant_propagation(*bin_expr->left);
        auto new_right = try_constant_propagation(*bin_expr->right);
        
        if (new_left || new_right) {
            // 处理左表达式
            std::unique_ptr<Expr> left_expr;
            if (new_left) {
                left_expr = std::move(new_left);
            } else {
                // 将 ASTNode 转换为 Expr
                auto cloned = bin_expr->left->clone();
                left_expr = std::unique_ptr<Expr>(static_cast<Expr*>(cloned.release()));
            }
            
            // 处理右表达式
            std::unique_ptr<Expr> right_expr;
            if (new_right) {
                right_expr = std::move(new_right);
            } else {
                // 将 ASTNode 转换为 Expr
                auto cloned = bin_expr->right->clone();
                right_expr = std::unique_ptr<Expr>(static_cast<Expr*>(cloned.release()));
            }
            
            auto new_bin_expr = std::make_unique<BinaryExpr>(
                bin_expr->op,
                std::move(left_expr),
                std::move(right_expr)
            );
            
            // 尝试常量折叠
            auto folded = constant_folding(*new_bin_expr);
            if (folded) {
                return folded;
            }
            
            return new_bin_expr;
        }
    }
    
    if (const auto* unary_expr = dynamic_cast<const UnaryExpr*>(&expr)) {
        auto new_operand = try_constant_propagation(*unary_expr->operand);
        
        if (new_operand) {
            auto new_unary_expr = std::make_unique<UnaryExpr>(
                unary_expr->op,
                std::move(new_operand)
            );
            
            // 尝试常量折叠
            auto folded = constant_folding(*new_unary_expr);
            if (folded) {
                return folded;
            }
            
            return new_unary_expr;
        }
    }
    
    // 处理 CallExpr（如果有常量参数，可以尝试传播）
    if (const auto* call_expr = dynamic_cast<const CallExpr*>(&expr)) {
        // 检查参数是否都是常量
        [[maybe_unused]] bool all_args_const = true;
        for (const auto& arg : call_expr->args) {
            if (!is_constant_expr(*arg)) {
                all_args_const = false;
                break;
            }
        }
        
        // 注意：函数调用通常有副作用，所以这里不进行传播
        // 但在某些情况下（纯函数），可以进行常量传播
    }
    
    return nullptr;
}

// 循环不变代码外提
void Optimizer::enter_loop() {
    loop_info_stack.push_back(LoopInfo{});
}

void Optimizer::exit_loop() {
    if (!loop_info_stack.empty()) {
        loop_info_stack.pop_back();
    }
}

void Optimizer::analyze_loop_invariant(WhileStmt& while_stmt) {
    if (loop_info_stack.empty()) {
        return;
    }
    
    // 分析条件表达式的变量依赖
    auto condition_vars = collect_variables(*while_stmt.condition);
    loop_info_stack.back().loop_variables.insert(condition_vars.begin(), condition_vars.end());
}

void Optimizer::hoist_loop_invariants(WhileStmt& while_stmt) {
    if (loop_info_stack.empty()) {
        return;
    }
    
    
    auto* body_block = dynamic_cast<Block*>(while_stmt.body.get());
    if (!body_block) {
        return;
    }
    
    std::vector<std::unique_ptr<Stmt>> new_statements;
    std::vector<std::unique_ptr<Stmt>> hoisted_statements;
    
    for (auto& stmt : body_block->statements) {
        // 检查是否为赋值语句，并且右侧表达式不依赖循环变量
        if (auto* assign_stmt = dynamic_cast<AssignStmt*>(stmt.get())) {
            if (!depends_on_loop_variables(*assign_stmt->value)) {
                // 可以外提
                hoisted_statements.push_back(std::move(stmt));
                record_optimization("loop invariant code motion - assignment");
            } else {
                new_statements.push_back(std::move(stmt));
            }
        } else if (auto* expr_stmt = dynamic_cast<ExprStmt*>(stmt.get())) {
            if (expr_stmt->expression && !depends_on_loop_variables(*expr_stmt->expression) && 
                !has_side_effects(*expr_stmt->expression)) {
                // 可以外提（无副作用的表达式）
                hoisted_statements.push_back(std::move(stmt));
                record_optimization("loop invariant code motion - expression");
            } else {
                new_statements.push_back(std::move(stmt));
            }
        } else if (auto* var_decl = dynamic_cast<VarDecl*>(stmt.get())) {
            // 变量声明：如果初始化表达式不依赖循环变量
            if (!var_decl->init_expr || 
                (!depends_on_loop_variables(*var_decl->init_expr) && 
                 !has_side_effects(*var_decl->init_expr))) {
                // 可以外提
                hoisted_statements.push_back(std::move(stmt));
                record_optimization("loop invariant code motion - variable declaration");
            } else {
                new_statements.push_back(std::move(stmt));
            }
        } else {
            new_statements.push_back(std::move(stmt));
        }
    }
    
    // 更新循环体
    body_block->statements = std::move(new_statements);
    
    // 在实际实现中，我们需要将外提的语句插入到循环前面
    // 这里只是记录，需要上层调用者处理
    loop_info_stack.back().hoisted_statements = std::move(hoisted_statements);
}

// 工具函数
std::set<std::string> Optimizer::collect_variables(const Expr& expr) const {
    std::set<std::string> vars;
    
    if (const auto* var_expr = dynamic_cast<const VarExpr*>(&expr)) {
        vars.insert(var_expr->name);
    } else if (const auto* bin_expr = dynamic_cast<const BinaryExpr*>(&expr)) {
        auto left_vars = collect_variables(*bin_expr->left);
        auto right_vars = collect_variables(*bin_expr->right);
        vars.insert(left_vars.begin(), left_vars.end());
        vars.insert(right_vars.begin(), right_vars.end());
    } else if (const auto* unary_expr = dynamic_cast<const UnaryExpr*>(&expr)) {
        auto operand_vars = collect_variables(*unary_expr->operand);
        vars.insert(operand_vars.begin(), operand_vars.end());
    } else if (const auto* call_expr = dynamic_cast<const CallExpr*>(&expr)) {
        for (const auto& arg : call_expr->args) {
            auto arg_vars = collect_variables(*arg);
            vars.insert(arg_vars.begin(), arg_vars.end());
        }
    }
    // NumberExpr 没有变量
    
    return vars;
}

bool Optimizer::depends_on_loop_variables(const Expr& expr) const {
    if (loop_info_stack.empty()) {
        return false;
    }
    
    auto vars = collect_variables(expr);
    const auto& loop_vars = loop_info_stack.back().loop_variables;
    
    for (const auto& var : vars) {
        if (loop_vars.find(var) != loop_vars.end()) {
            return true;
        }
    }
    
    return false;
}

bool Optimizer::is_constant_expr(const Expr& expr) const {
    if (dynamic_cast<const NumberExpr*>(&expr)) {
        return true;
    }
    
    if (auto* var_expr = dynamic_cast<const VarExpr*>(&expr)) {
        return is_variable_constant.count(var_expr->name) > 0;
    }
    
    if (auto* bin_expr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return is_constant_expr(*bin_expr->left) && is_constant_expr(*bin_expr->right);
    }
    
    if (auto* unary_expr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return is_constant_expr(*unary_expr->operand);
    }
    
    // CallExpr 通常不是常量表达式（除非是纯函数）
    return false;
}

int Optimizer::evaluate_constant_expr(const Expr& expr) const {
    if (auto* num_expr = dynamic_cast<const NumberExpr*>(&expr)) {
        return num_expr->value;
    }
    
    // 常量传播：如果变量有常量值，使用该值
    if (auto* var_expr = dynamic_cast<const VarExpr*>(&expr)) {
        auto it = constant_values.find(var_expr->name);
        if (it != constant_values.end()) {
            if (auto* num_expr = dynamic_cast<const NumberExpr*>(it->second.get())) {
                return num_expr->value;
            }
        }
        return 0;  // 未知变量，假设为0
    }
    
    if (auto* bin_expr = dynamic_cast<const BinaryExpr*>(&expr)) {
        int left_val = evaluate_constant_expr(*bin_expr->left);
        int right_val = evaluate_constant_expr(*bin_expr->right);
        
        switch (bin_expr->op) {
            case BinaryExpr::Operator::ADD: 
                return left_val + right_val;
            case BinaryExpr::Operator::SUB: 
                return left_val - right_val;
            case BinaryExpr::Operator::MUL: 
                return left_val * right_val;
            case BinaryExpr::Operator::DIV: 
                return right_val != 0 ? left_val / right_val : 0;
            case BinaryExpr::Operator::MOD: 
                return right_val != 0 ? left_val % right_val : 0;
            case BinaryExpr::Operator::LT: 
                return left_val < right_val ? 1 : 0;
            case BinaryExpr::Operator::GT: 
                return left_val > right_val ? 1 : 0;
            case BinaryExpr::Operator::LE: 
                return left_val <= right_val ? 1 : 0;
            case BinaryExpr::Operator::GE: 
                return left_val >= right_val ? 1 : 0;
            case BinaryExpr::Operator::EQ: 
                return left_val == right_val ? 1 : 0;
            case BinaryExpr::Operator::NE: 
                return left_val != right_val ? 1 : 0;
            case BinaryExpr::Operator::AND: 
                return (left_val && right_val) ? 1 : 0;
            case BinaryExpr::Operator::OR: 
                return (left_val || right_val) ? 1 : 0;
            default: 
                return 0;
        }
    }
    
    if (auto* unary_expr = dynamic_cast<const UnaryExpr*>(&expr)) {
        int operand_val = evaluate_constant_expr(*unary_expr->operand);
        
        switch (unary_expr->op) {
            case UnaryExpr::Operator::PLUS: 
                return operand_val;
            case UnaryExpr::Operator::MINUS: 
                return -operand_val;
            case UnaryExpr::Operator::NOT: 
                return operand_val ? 0 : 1;
            default: 
                return 0;
        }
    }
    
    return 0;
}

bool Optimizer::has_side_effects(const Expr& expr) const {
    // 函数调用有副作用
    if (dynamic_cast<const CallExpr*>(&expr)) {
        return true;
    }
    
    // 其他表达式类型在ToyC中没有副作用
    if (auto* bin_expr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return has_side_effects(*bin_expr->left) || has_side_effects(*bin_expr->right);
    }
    
    if (auto* unary_expr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return has_side_effects(*unary_expr->operand);
    }
    
    return false;  // 变量引用、数字字面量等没有副作用
}

void Optimizer::record_optimization(const std::string& type) {
    optimizations_applied++;
    
    #ifdef DEBUG_OPTIMIZATION
    std::cerr << "Applied optimization: " << type << std::endl;
    #else
    (void)type;
    #endif
}
