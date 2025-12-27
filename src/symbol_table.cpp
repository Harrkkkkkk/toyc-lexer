#include "symbol_table.h"
#include <iostream>
#include <iomanip>

// Symbol 实现
Symbol::Symbol(const std::string& sym_name, SymbolType sym_type, 
               DataType dt, int scope)
    : name(sym_name), symbol_type(sym_type), data_type(dt), scope_level(scope) {}

// SymbolTable 静态工具函数实现
std::string SymbolTable::data_type_to_string(DataType type) {
    switch (type) {
        case DataType::INT: return "int";
        case DataType::VOID: return "void";
        default: return "unknown";
    }
}

std::string SymbolTable::symbol_type_to_string(SymbolType type) {
    switch (type) {
        case SymbolType::VARIABLE: return "variable";
        case SymbolType::FUNCTION: return "function";
        case SymbolType::PARAMETER: return "parameter";
        default: return "unknown";
    }
}

// SymbolTable 构造函数
SymbolTable::SymbolTable() 
    : global_scope(std::make_unique<Scope>(0)), 
      current_scope(global_scope.get()), 
      next_scope_level(1) {
    scope_stack_size.push_back(0);  // 全局作用域初始栈大小为0
}

// SymbolTable 作用域管理
void SymbolTable::enter_scope() {
    auto new_scope = std::make_unique<Scope>(next_scope_level++, current_scope);
    current_scope = new_scope.release();  
    scope_stack_size.push_back(0);  // 新作用域初始栈大小为0
}

void SymbolTable::exit_scope() {
    if (current_scope->parent) {
        Scope* old_scope = current_scope;
        current_scope = current_scope->parent;
        delete old_scope;  
        scope_stack_size.pop_back();
    }
}

int SymbolTable::get_current_scope_level() const {
    return current_scope->level;
}

// SymbolTable 符号定义和查找
bool SymbolTable::define_variable(const std::string& name, DataType type) {
    auto symbol = std::make_unique<Symbol>(name, SymbolType::VARIABLE, type, current_scope->level);
    
    // 为变量分配栈空间
    allocate_stack_space(symbol.get());
    
    return current_scope->define(std::move(symbol));
}

bool SymbolTable::define_function(const std::string& name, DataType return_type, 
                                 const std::vector<DataType>& param_types) {
    auto symbol = std::make_unique<Symbol>(name, SymbolType::FUNCTION, return_type, 0);  // 函数总是在全局作用域
    symbol->param_types = param_types;
    
    return global_scope->define(std::move(symbol));
}

bool SymbolTable::define_parameter(const std::string& name, DataType type) {
    auto symbol = std::make_unique<Symbol>(name, SymbolType::PARAMETER, type, current_scope->level);
    
    allocate_stack_space(symbol.get());
    
    return current_scope->define(std::move(symbol));
}

Symbol* SymbolTable::lookup_symbol(const std::string& name) {
    return current_scope->lookup(name);
}

Symbol* SymbolTable::lookup_function(const std::string& name) {
    Symbol* symbol = global_scope->lookup_local(name);
    if (symbol && symbol->symbol_type == SymbolType::FUNCTION) {
        return symbol;
    }
    return nullptr;
}

// SymbolTable 栈管理
int SymbolTable::get_current_stack_size() const {
    if (scope_stack_size.empty()) {
        return 0;
    }
    return scope_stack_size.back();
}

void SymbolTable::allocate_stack_space(Symbol* symbol) {
    if (symbol->symbol_type == SymbolType::VARIABLE) {
        // 局部变量：负偏移
        scope_stack_size.back()++;
        symbol->stack_offset = -scope_stack_size.back() * 4;  
    } else if (symbol->symbol_type == SymbolType::PARAMETER) {
        // 参数：正偏移
        scope_stack_size.back()++;
        symbol->stack_offset = scope_stack_size.back() * 4;
    }
}

// SymbolTable 调试输出
void SymbolTable::print_current_scope() const {
    std::cout << "=== Current Scope (Level " << current_scope->level << ") ===\n";
    print_scope(current_scope);
    std::cout << "================================\n";
}

void SymbolTable::print_all_scopes() const {
    std::cout << "=== All Scopes ===\n";
    print_scope(global_scope.get());
    std::cout << "==================\n";
}

void SymbolTable::find_and_print_subscopes(const Scope* parent_scope, int indent) const {
    // 由于没有存储子作用域的引用，这里只能打印一个简单的说明
    if (!parent_scope) return;
    
    for (int i = 0; i < indent; i++) {
        std::cout << "  ";
    }
    std::cout << "[Sub-scopes: to print all sub-scopes, modify Scope class to store child scopes]\n";
}

void SymbolTable::print_scope(const Scope* scope, int indent) const {
    if (!scope) return;
    
    for (int i = 0; i < indent; i++) {
        std::cout << "  ";
    }
    std::cout << "Scope Level " << scope->level << ":\n";
    
    for (const auto& pair : scope->symbols) {
        const Symbol* symbol = pair.second.get();
        for (int i = 0; i < indent + 1; i++) {
            std::cout << "  ";
        }
        std::cout << symbol->name << " (" 
                  << symbol_type_to_string(symbol->symbol_type) << ", "
                  << data_type_to_string(symbol->data_type);
        
        if (symbol->symbol_type == SymbolType::FUNCTION) {
            std::cout << ", params: [";
            for (size_t i = 0; i < symbol->param_types.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << data_type_to_string(symbol->param_types[i]);
            }
            std::cout << "]";
        } else if (symbol->symbol_type != SymbolType::FUNCTION) {
            std::cout << ", offset: " << symbol->stack_offset;
        }
        
        std::cout << ")\n";
    }
    
    // 检查是否有子作用域
    // 注意：由于使用链表结构，需要通过遍历所有作用域来查找子作用域
    // 这里使用一个辅助方法来查找子作用域
    if (indent == 0) {  // 只在顶层查找子作用域
        // 查找所有子作用域
        find_and_print_subscopes(scope, indent + 1);
    }
}

// Scope 实现
Scope::Scope(int scope_level, Scope* parent_scope)
    : level(scope_level), parent(parent_scope) {}

Symbol* Scope::lookup_local(const std::string& name) {
    auto it = symbols.find(name);
    return (it != symbols.end()) ? it->second.get() : nullptr;
}

Symbol* Scope::lookup(const std::string& name) {
    // 先在当前作用域查找
    Symbol* symbol = lookup_local(name);
    if (symbol) {
        return symbol;
    }
    
    // 在父作用域中递归查找
    if (parent) {
        return parent->lookup(name);
    }
    
    return nullptr;
}

bool Scope::define(std::unique_ptr<Symbol> symbol) {
    const std::string& name = symbol->name;
    
    // 检查当前作用域是否已定义
    if (is_defined_local(name)) {
        return false;  // 重复定义
    }
    
    symbols[name] = std::move(symbol);
    return true;
}

bool Scope::is_defined_local(const std::string& name) {
    return symbols.find(name) != symbols.end();
}
