#pragma once
#include "parser/ast.h"
#include <vector>
#include <string>
#include <memory>
#include <map>

// ==================== 枚举和结构体定义 ====================

enum class OperandType {
    VARIABLE,
    TEMP,
    CONSTANT,
    LABEL
};

enum class OpCode {
    ADD, SUB, MUL, DIV, MOD,
    NEG, NOT,
    LT, GT, LE, GE, EQ, NE,
    AND, OR,
    SHL, SHR,  // 左移和右移
    ASSIGN,
    GOTO, IF_GOTO,
    PARAM, CALL, RETURN,
    LABEL,
    FUNCTION_BEGIN, FUNCTION_END
};

// ==================== 操作数类 ====================

class Operand {
public:
    OperandType type;
    std::string name;
    int value;

    Operand(OperandType type, const std::string& name) : type(type), name(name), value(0) {}
    Operand(int value) : type(OperandType::CONSTANT), value(value) {}

    std::string toString() const;
    bool isTemp() const { return type == OperandType::TEMP; }
};

bool isProcessableReg(const Operand& op);
std::vector<std::string> extractReg(const std::shared_ptr<Operand>& op);
std::vector<std::string> collectRegs(const std::initializer_list<std::shared_ptr<Operand>>& ops);

// ==================== IR指令基类 ====================

class IRInstr {
public:
    OpCode opcode;
    
    IRInstr(OpCode opcode) : opcode(opcode) {}
    virtual ~IRInstr();
    virtual std::string toString() const = 0;

    virtual std::vector<std::string> getDefRegisters();
    virtual std::vector<std::string> getUseRegisters();
};

// ==================== 具体指令类 ====================

class BinaryOpInstr : public IRInstr {
public:
    std::shared_ptr<Operand> result;
    std::shared_ptr<Operand> left;
    std::shared_ptr<Operand> right;
    
    BinaryOpInstr(OpCode opcode, 
                 std::shared_ptr<Operand> result,
                 std::shared_ptr<Operand> left,
                 std::shared_ptr<Operand> right)
        : IRInstr(opcode), result(result), left(left), right(right) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return extractReg(result);
    }
    
    std::vector<std::string> getUseRegisters() override {
        return collectRegs({left, right});
    }
};

class UnaryOpInstr : public IRInstr {
public:
    std::shared_ptr<Operand> result;
    std::shared_ptr<Operand> operand;
    
    UnaryOpInstr(OpCode opcode,
                std::shared_ptr<Operand> result,
                std::shared_ptr<Operand> operand)
        : IRInstr(opcode), result(result), operand(operand) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return extractReg(result);
    }

    std::vector<std::string> getUseRegisters() override {
        return extractReg(operand);
    }
};

class AssignInstr : public IRInstr {
public:
    std::shared_ptr<Operand> target;
    std::shared_ptr<Operand> source;
    
    AssignInstr(std::shared_ptr<Operand> target,
               std::shared_ptr<Operand> source)
        : IRInstr(OpCode::ASSIGN), target(target), source(source) {}
    
    std::string toString() const override;

    bool isSimpleCopy() const {
        return (source->type == OperandType::VARIABLE || source->type == OperandType::TEMP);
    }

    std::vector<std::string> getDefRegisters() override {
        return extractReg(target);
    }
    
    std::vector<std::string> getUseRegisters() override {
        return extractReg(source);
    }
};

class GotoInstr : public IRInstr {
public:
    std::shared_ptr<Operand> target;
    
    GotoInstr(std::shared_ptr<Operand> target)
        : IRInstr(OpCode::GOTO), target(target) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return {};
    }
        
    std::vector<std::string> getUseRegisters() override {
        return {};
    }
};

class IfGotoInstr : public IRInstr {
public:
    std::shared_ptr<Operand> condition;
    std::shared_ptr<Operand> target;
    
    IfGotoInstr(std::shared_ptr<Operand> condition,
               std::shared_ptr<Operand> target)
        : IRInstr(OpCode::IF_GOTO), condition(condition), target(target) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return {};
    }
            
    std::vector<std::string> getUseRegisters() override {
        return extractReg(condition);
    }
};

class ParamInstr : public IRInstr {
public:
    std::shared_ptr<Operand> param;
    
    ParamInstr(std::shared_ptr<Operand> param)
        : IRInstr(OpCode::PARAM), param(param) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return {};
    }
            
    std::vector<std::string> getUseRegisters() override {
        return extractReg(param);
    }
};

class CallInstr : public IRInstr {
public:
    std::shared_ptr<Operand> result;
    std::string funcName;
    int paramCount;
    std::vector<std::shared_ptr<Operand>> params;

    CallInstr(std::shared_ptr<Operand> result,
             const std::string& funcName,
             int paramCount)
        : IRInstr(OpCode::CALL), result(result), funcName(funcName), paramCount(paramCount) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return extractReg(result);
    }

    std::vector<std::string> getUseRegisters() override {
        std::vector<std::string> regs;
        for (const auto& param : params) {
            auto r = extractReg(param);
            regs.insert(regs.end(), r.begin(), r.end());
        }
        return regs;
    }
};

class ReturnInstr : public IRInstr {
public:
    std::shared_ptr<Operand> value;
    
    ReturnInstr(std::shared_ptr<Operand> value = nullptr)
        : IRInstr(OpCode::RETURN), value(value) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return {};
    }
                
    std::vector<std::string> getUseRegisters() override {
        return extractReg(value);
    }
};

class LabelInstr : public IRInstr {
public:
    std::string label;
    
    LabelInstr(const std::string& label)
        : IRInstr(OpCode::LABEL), label(label) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return {};
    }
                
    std::vector<std::string> getUseRegisters() override {
        return {};
    }
};

class FunctionBeginInstr : public IRInstr {
public:
    std::string funcName;
    std::vector<std::string> paramNames;
    std::string returnType;
    
    FunctionBeginInstr(const std::string& funcName, const std::string& returnType = "int")
        : IRInstr(OpCode::FUNCTION_BEGIN), funcName(funcName), returnType(returnType) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return {};
    }
                
    std::vector<std::string> getUseRegisters() override {
        return {};
    }
};

class FunctionEndInstr : public IRInstr {
public:
    std::string funcName;
    
    FunctionEndInstr(const std::string& funcName)
        : IRInstr(OpCode::FUNCTION_END), funcName(funcName) {}
    
    std::string toString() const override;

    std::vector<std::string> getDefRegisters() override {
        return {};
    }
                
    std::vector<std::string> getUseRegisters() override {
        return {};
    }
};

// ==================== IR工具类 ====================

class IRPrinter {
public:
    static void print(const std::vector<std::shared_ptr<IRInstr>>& instructions, std::ostream& out);
};

class IRAnalyzer {
public:
    static int findDefinition(const std::vector<std::shared_ptr<IRInstr>>& instructions, 
                             const std::string& operandName);
                             
    static std::vector<int> findUses(const std::vector<std::shared_ptr<IRInstr>>& instructions, 
                                   const std::string& operandName);
                                   
    static bool isVariableLive(const std::vector<std::shared_ptr<IRInstr>>& instructions,
                              const std::string& varName,
                              int position);
                              
    static std::vector<std::string> getDefinedVariables(const std::shared_ptr<IRInstr>& instr);
    
    static std::vector<std::string> getUsedVariables(const std::shared_ptr<IRInstr>& instr);

    static bool isFunctionUsed(const std::vector<std::shared_ptr<IRInstr>>& instructions,
                          const std::string& funcName);

    static void replaceUsedVariable(std::shared_ptr<IRInstr>& instr, 
                            const std::string& oldVar, const std::shared_ptr<Operand>& newOp);
};