#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// 数据类型
enum class DataType {
    INT,
    VOID
};

// 符号类型
enum class SymbolType {
    VARIABLE,
    FUNCTION,
    PARAMETER
};

// 符号信息
class Symbol {
public:
    std::string name;
    SymbolType symbol_type;
    DataType data_type;
    int scope_level;
    
    // 对于变量：栈偏移量
    int stack_offset = 0;
    
    // 对于函数：参数类型列表
    std::vector<DataType> param_types;
    
    Symbol(const std::string& sym_name, SymbolType sym_type, 
           DataType dt, int scope = 0);
};

// 作用域管理
class Scope {
public:
    int level;
    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;
    Scope* parent;
    
    explicit Scope(int scope_level, Scope* parent_scope = nullptr);
    
    // 在当前作用域中查找符号
    Symbol* lookup_local(const std::string& name);
    
    // 在当前作用域及其父作用域中查找符号
    Symbol* lookup(const std::string& name);
    
    // 在当前作用域中定义符号
    bool define(std::unique_ptr<Symbol> symbol);
    
    // 检查当前作用域是否已定义该符号
    bool is_defined_local(const std::string& name);
};

// 符号表管理器
class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable() = default;
    
    // 作用域管理
    void enter_scope();
    void exit_scope();
    int get_current_scope_level() const;
    
    // 符号定义和查找
    bool define_variable(const std::string& name, DataType type);
    bool define_function(const std::string& name, DataType return_type, 
                        const std::vector<DataType>& param_types);
    bool define_parameter(const std::string& name, DataType type);
    
    Symbol* lookup_symbol(const std::string& name);
    Symbol* lookup_function(const std::string& name);
    
    // 栈管理
    int get_current_stack_size() const;
    void allocate_stack_space(Symbol* symbol);
    
    // 工具函数
    static std::string data_type_to_string(DataType type);
    static std::string symbol_type_to_string(SymbolType type);
    
    // 调试输出
    void print_current_scope() const;
    void print_all_scopes() const;
    
private:
    // 用于栈管理的变量
    std::vector<int> scope_stack_size;  // 每个作用域的局部变量数量
    
private:
    std::unique_ptr<Scope> global_scope;
    Scope* current_scope;
    int next_scope_level;
    
private:
    void print_scope(const Scope* scope, int indent = 0) const;
    void find_and_print_subscopes(const Scope* parent_scope, int indent) const;
};