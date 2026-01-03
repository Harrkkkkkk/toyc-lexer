#pragma once
#include <string>
#include <vector>

struct Symbol
{
    enum class Kind { VARIABLE, FUNCTION, PARAMETER };
    
    Kind kind;
    std::string type;
    int line;
    int column;
    int paramIndex = -1;
    std::vector<std::pair<std::string, std::string>> params;
    bool used = false;
    
    Symbol() = default;
    
    Symbol(Kind kind, const std::string &type, int line = 0, int column = 0, int paramIndex = -1)
        : kind(kind), type(type), line(line), column(column), paramIndex(paramIndex), used(false) {}
        
    Symbol(Kind kind, const std::string &type, 
           const std::vector<std::pair<std::string, std::string>>& parameters,
           int line = 0, int column = 0)
        : kind(kind), type(type), line(line), column(column), 
          paramIndex(-1), params(parameters), used(false) {}
};

struct FunctionInfo
{
    std::string returnType;
    std::vector<std::string> paramTypes;
    std::vector<std::string> paramNames;
    int line;
    int column;
    bool used = false;
    
    FunctionInfo(const std::string &returnType = "void", int line = 0, int column = 0)
        : returnType(returnType), line(line), column(column), used(false) {}
};

struct OptionalInt {
    bool hasValue;
    int value;
    
    OptionalInt() : hasValue(false), value(0) {}
    OptionalInt(int v) : hasValue(true), value(v) {}
    
    bool has_value() const { return hasValue; }
    int operator*() const { return value; }
    explicit operator bool() const { return hasValue; }
};

class SemanticError : public std::runtime_error {
public:
    int line;
    int column;
    SemanticError(const std::string &message, int line = 0, int column = 0)
        : std::runtime_error(message), line(line), column(column) {}
};