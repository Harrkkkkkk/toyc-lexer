#pragma once
#include "parser/ast.h"
#include "ir/ir.h"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <memory>
#include <functional>

// ==================== 枚举和结构体定义 ====================

enum class RegisterAllocStrategy {
    NAIVE,
    LINEAR_SCAN,
    GRAPH_COLOR
};

struct CodeGenConfig {
    bool optimizeStackLayout = false;
    bool eliminateDeadStores = false;
    bool enablePeepholeOptimizations = false;
    bool enableInlineAsm = false;
    RegisterAllocStrategy regAllocStrategy = RegisterAllocStrategy::NAIVE;
};

struct Register {
    std::string name;
    bool isCallerSaved;
    bool isCalleeSaved;
    bool isAllocatable;
    bool isReserved;
    std::string purpose;
    bool isUsed;
};

// ==================== 寄存器分配器基类 ====================

class RegisterAllocator {
public:
    virtual ~RegisterAllocator() = default;
    virtual std::map<std::string, std::string> allocate(
        const std::vector<std::shared_ptr<IRInstr>>& instructions,
        const std::vector<Register>& availableRegs) = 0;
};

// ==================== 简单寄存器分配器 ====================

class NaiveRegisterAllocator : public RegisterAllocator {
public:
    std::map<std::string, std::string> allocate(
        const std::vector<std::shared_ptr<IRInstr>>& instructions,
        const std::vector<Register>& availableRegs) override;
};

// ==================== 线性扫描寄存器分配器 ====================

class LinearScanRegisterAllocator : public RegisterAllocator {
public:
    std::map<std::string, std::string> allocate(
        const std::vector<std::shared_ptr<IRInstr>>& instructions,
        const std::vector<Register>& availableRegs) override;
    
private:
    struct LiveInterval {
        std::string var;
        int start;
        int end;
        
        bool operator<(const LiveInterval& other) const {
            return start < other.start;
        }
    };
    
    std::vector<LiveInterval> computeLiveIntervals(
        const std::vector<std::shared_ptr<IRInstr>>& instructions);
};

// ==================== 图着色寄存器分配器 ====================

class GraphColoringRegisterAllocator : public RegisterAllocator {
public:
    std::map<std::string, std::string> allocate(
        const std::vector<std::shared_ptr<IRInstr>>& instructions,
        const std::vector<Register>& availableRegs) override;
    
private:
    std::map<std::string, std::set<std::string>> buildInterferenceGraph(
        const std::vector<std::shared_ptr<IRInstr>>& instructions);
    
    std::vector<std::string> simplify(
        std::map<std::string, std::set<std::string>>& graph);
    
    std::map<std::string, std::string> color(
        const std::vector<std::string>& simplifiedOrder,
        const std::map<std::string, std::set<std::string>>& originalGraph,
        const std::vector<Register>& availableRegs);
};

// ==================== 代码生成器主类 ====================

class CodeGenerator {
private:
    std::ostream& output;
    const std::vector<std::shared_ptr<IRInstr>>& instructions;
    CodeGenConfig config;
    
    // 寄存器信息
    std::vector<Register> registers;
    std::vector<std::string> tempRegs = {"t0", "t1", "t2", "t3", "t4", "t5", "t6"};
    std::vector<std::string> argRegs = {"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
    int nextTempReg = 0;
    
    // 栈和变量管理
    std::map<std::string, int> localVars;
    std::map<std::string, std::string> regAlloc;
    std::set<std::string> activeVars;
    std::map<std::string, int> regOffsetMap;
    std::set<std::string> usedCalleeSavedRegs;
    std::set<std::string> usedCallerSavedRegs;
    
    // 函数上下文
    std::string currentFunction;
    std::string currentFunctionReturnType;
    std::vector<std::string> currentFunctionParams;
    std::vector<std::shared_ptr<Operand>> paramQueue;
    
    // 栈状态
    int frameSize = 0;
    int localVarsSize = 0;
    int calleeRegsSize = 0;
    int callerRegsSize = 0;
    int paramStackSize = 0;
    int currentStackOffset = 0;
    bool frameInitialized = false;
    
    // 控制流状态
    bool isInLoop = false;
    std::vector<std::string> breakLabels;
    std::vector<std::string> continueLabels;
    int labelCount = 0;
    
    // 优化
    std::map<std::string, std::function<bool(std::vector<std::string>&)>> peepholePatterns;

public:
    CodeGenerator(std::ostream& outputStream,  
                 const std::vector<std::shared_ptr<IRInstr>>& instructions,
                 const CodeGenConfig& config = CodeGenConfig());
    ~CodeGenerator();
    
    void generate();
    void processInstructionToStream(const std::shared_ptr<IRInstr>& instr, std::ostream& stream);
    void addPeepholePattern(const std::string& pattern, 
                           std::function<bool(std::vector<std::string>&)> handler);

private:
    // 标签和输出
    std::string genLabel();
    void emitComment(const std::string& comment);
    void emitInstruction(const std::string& instr);
    void emitLabel(const std::string& label);
    void emitGlobal(const std::string& name);
    void emitSection(const std::string& section);
    
    // 指令处理
    void processInstruction(const std::shared_ptr<IRInstr>& instr);
    void processBinaryOp(const std::shared_ptr<BinaryOpInstr>& instr);
    void processUnaryOp(const std::shared_ptr<UnaryOpInstr>& instr);
    void processAssign(const std::shared_ptr<AssignInstr>& instr);
    void processGoto(const std::shared_ptr<GotoInstr>& instr);
    void processIfGoto(const std::shared_ptr<IfGotoInstr>& instr);
    void processParam(const std::shared_ptr<ParamInstr>& instr);
    void processCall(const std::shared_ptr<CallInstr>& instr);
    void processReturn(const std::shared_ptr<ReturnInstr>& instr);
    void processLabel(const std::shared_ptr<LabelInstr>& instr);
    void processFunctionBegin(const std::shared_ptr<FunctionBeginInstr>& instr);
    void processFunctionEnd(const std::shared_ptr<FunctionEndInstr>& instr);
    
    // 操作数和寄存器处理
    void loadOperand(const std::shared_ptr<Operand>& op, const std::string& reg);
    void storeRegister(const std::string& reg, const std::shared_ptr<Operand>& op);
    int getOperandOffset(const std::shared_ptr<Operand>& op);
    std::string allocTempReg();
    void freeTempReg(const std::string& reg);
    
    // 寄存器保存和恢复
    void saveCallerSavedRegs();
    void restoreCallerSavedRegs();
    void saveCalleeSavedRegs();
    void restoreCalleeSavedRegs();
    
    // 寄存器管理
    void initializeRegisters();
    void resetStackOffset();
    void allocateRegisters();
    bool isValidRegister(const std::string& reg) const;
    std::string getArgRegister(int paramIndex) const;
    void analyzeUsedCalleeSavedRegs();
    void analyzeUsedCallerSavedRegs();
    int countUsedCallerSavedRegs();
    int countUsedCalleeSavedRegs();
    int getRegisterStackOffset(const std::string& reg);
    
    // 栈帧管理
    void emitPrologue(const std::string& funcName);
    void emitEpilogue(const std::string& funcName);
    
    // 大小计算
    int getCallerSavedRegsSize() const { return usedCallerSavedRegs.size() * 4; }
    int getCalleeSavedRegsSize() const { return usedCalleeSavedRegs.size() * 4; }
    int getTotalFrameSize() const {
        int total = getCalleeSavedRegsSize() + localVarsSize + 8;
        return (total + 15) & ~15;
    }
    int getLocalVarsSize() const { return localVarsSize; }
    void incrementLocalVarsSize(int size) { localVarsSize += size; }
    
    // 优化方法
    void optimizeStackLayout();
    void peepholeOptimize(std::vector<std::string>& instructions);
    void linearScanRegisterAllocation();
    void graphColoringRegisterAllocation();
    
    // 分析方法
    void analyzeVariableLifetimes(std::map<std::string, std::pair<int, int>>& varLifetimes);
    int analyzeTempVars();
    std::map<std::string, std::set<std::string>> buildInterferenceGraph();
    
    // 辅助方法
    bool isTempReg(const std::string& reg) { return reg[0] == 't'; }
    bool isRegisterAllocated(const std::string& reg) {
        return regAlloc.find(reg) != regAlloc.end();
    }
};