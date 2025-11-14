#ifndef TOYC_SYMBOL_H
#define TOYC_SYMBOL_H

#include <string>
#include <unordered_map>
#include <vector>

struct VarInfo {
    std::string name;
    int declared_line;
};

struct FuncInfo {
    std::string name;
    bool returns_int;
    std::vector<std::string> params; // param names in order
    int declared_line;
};

class SymTable {
public:
    SymTable() { push_scope(); }
    void push_scope() { scopes.emplace_back(); }
    void pop_scope() { if(!scopes.empty()) scopes.pop_back(); }
    bool declare_var(const std::string &name, int line) {
        auto &cur = scopes.back();
        if (cur.count(name)) return false;
        cur[name] = VarInfo{name, line};
        return true;
    }
    bool has_var(const std::string &name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(name)) return true;
        }
        return false;
    }
    int var_decl_line(const std::string &name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto f = it->find(name);
            if (f != it->end()) return f->second.declared_line;
        }
        return -1;
    }
private:
    std::vector<std::unordered_map<std::string, VarInfo>> scopes;
};

#endif
