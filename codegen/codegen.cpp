#include "codegen.h"
#include <sstream>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <queue>
#include <stack>
#include <limits>

// ==================== 构造函数和析构函数 ====================

CodeGenerator::CodeGenerator(std::ostream& outputStream,  
                           const std::vector<std::shared_ptr<IRInstr>>& instructions,
                           const CodeGenConfig& config)
    : output(outputStream), instructions(instructions), config(config) {
    
    // std::cerr << "CodeGenerator构造函数开始\n";
    // std::cerr << "IR指令数量: " << instructions.size() << "\n";
    // std::cerr << "初始化寄存器信息\n";

    initializeRegisters();
    // std::cerr << "寄存器信息初始化完成\n";
    // std::cerr << "输出文件头\n";

    emitComment("由ToyC编译器生成");
    emitComment("RISC-V汇编代码");
    emitSection(".text");
    
    if (config.regAllocStrategy != RegisterAllocStrategy::NAIVE) {
        // std::cerr << "执行寄存器分配\n";
        allocateRegisters();
        // std::cerr << "寄存器分配完成\n";
    }
    // std::cerr << "CodeGenerator构造函数完成\n";
}

CodeGenerator::~CodeGenerator() {
}

// ==================== 主要生成函数 ====================

void CodeGenerator::generate() {
    // std::cerr << "进入generate方法\n";

    std::vector<std::string> asmInstructions;

    // std::cerr << "开始处理IR指令, 总数: " << instructions.size() << "\n";
    for (const auto& instr : instructions) {
        std::stringstream tempOutput;
        std::streampos originalPos = output.tellp();
        
        processInstructionToStream(instr, tempOutput);
        
        std::string asmCode = tempOutput.str();
        std::istringstream iss(asmCode);
        std::string line;
        
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                if (line[0] != '#' && line[0] != '\t' && line.back() != ':') {
                    asmInstructions.push_back(line);
                } else {
                    output << line << "\n";
                }
            }
        }
    }
    
    // std::cerr << "IR指令处理完成\n";

    if (config.enablePeepholeOptimizations) {
        peepholeOptimize(asmInstructions);
    }
    
    if (config.optimizeStackLayout) {
        optimizeStackLayout();
    }
    
    for (const auto& instr : asmInstructions) {
        output << "\t" << instr << "\n";
    }

    // std::cerr << "generate方法执行完成\n";
}

// ==================== 指令流处理 ====================

void CodeGenerator::processInstructionToStream(const std::shared_ptr<IRInstr>& instr, std::ostream& stream) {
    std::streambuf* originalBuf = output.std::ostream::rdbuf();
    output.std::ostream::rdbuf(stream.rdbuf());
    processInstruction(instr);
    output.std::ostream::rdbuf(originalBuf);
}

// ==================== 指令处理函数 ====================

void CodeGenerator::processInstruction(const std::shared_ptr<IRInstr>& instr) {
    switch (instr->opcode) {
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::MOD:
        case OpCode::LT:
        case OpCode::GT:
        case OpCode::LE:
        case OpCode::GE:
        case OpCode::EQ:
        case OpCode::NE:
        case OpCode::AND:
        case OpCode::OR:
        case OpCode::SHL:
        case OpCode::SHR:
            processBinaryOp(std::dynamic_pointer_cast<BinaryOpInstr>(instr));
            break;
            
        case OpCode::NEG:
        case OpCode::NOT:
            processUnaryOp(std::dynamic_pointer_cast<UnaryOpInstr>(instr));
            break;
            
        case OpCode::ASSIGN:
            processAssign(std::dynamic_pointer_cast<AssignInstr>(instr));
            break;
            
        case OpCode::GOTO:
            processGoto(std::dynamic_pointer_cast<GotoInstr>(instr));
            break;
            
        case OpCode::IF_GOTO:
            processIfGoto(std::dynamic_pointer_cast<IfGotoInstr>(instr));
            break;
            
        case OpCode::PARAM:
            processParam(std::dynamic_pointer_cast<ParamInstr>(instr));
            break;
            
        case OpCode::CALL:
            processCall(std::dynamic_pointer_cast<CallInstr>(instr));
            break;
            
        case OpCode::RETURN:
            processReturn(std::dynamic_pointer_cast<ReturnInstr>(instr));
            break;
            
        case OpCode::LABEL:
            processLabel(std::dynamic_pointer_cast<LabelInstr>(instr));
            break;
            
        case OpCode::FUNCTION_BEGIN:
            processFunctionBegin(std::dynamic_pointer_cast<FunctionBeginInstr>(instr));
            break;
            
        case OpCode::FUNCTION_END:
            processFunctionEnd(std::dynamic_pointer_cast<FunctionEndInstr>(instr));
            break;
            
        default:
            std::cerr << "Error: Unknown instruction type" << std::endl;
            break;
    }
}

// ==================== 具体指令类型处理 ====================

void CodeGenerator::processBinaryOp(const std::shared_ptr<BinaryOpInstr>& instr) {
    emitComment(instr->toString());
    std::string resultReg = allocTempReg();

    if (instr->opcode == OpCode::AND || instr->opcode == OpCode::OR) {
        std::string leftReg = allocTempReg();
        loadOperand(instr->left, leftReg);

        std::string endLabel = genLabel();
        std::string shortCircuitLabel = genLabel();

        if (instr->opcode == OpCode::AND) {
            emitInstruction("beqz " + leftReg + ", " + shortCircuitLabel);
            std::string rightReg = allocTempReg();
            loadOperand(instr->right, rightReg);
            emitInstruction("snez " + resultReg + ", " + rightReg);
            freeTempReg(rightReg);
            emitInstruction("j " + endLabel);
            emitLabel(shortCircuitLabel);
            emitInstruction("li " + resultReg + ", 0");
        } else {
            emitInstruction("bnez " + leftReg + ", " + shortCircuitLabel);
            std::string rightReg = allocTempReg();
            loadOperand(instr->right, rightReg);
            emitInstruction("snez " + resultReg + ", " + rightReg);
            freeTempReg(rightReg);
            emitInstruction("j " + endLabel);
            emitLabel(shortCircuitLabel);
            emitInstruction("li " + resultReg + ", 1");
        }

        emitLabel(endLabel);
        freeTempReg(leftReg);       
    } else {
        std::string leftReg = allocTempReg();
        std::string rightReg = allocTempReg();

        loadOperand(instr->left, leftReg);
        loadOperand(instr->right, rightReg);

        switch (instr->opcode) {
            case OpCode::ADD:
                emitInstruction("add " + resultReg + ", " + leftReg + ", " + rightReg);
                break;
            case OpCode::SUB:
                emitInstruction("sub " + resultReg + ", " + leftReg + ", " + rightReg);
                break;
            case OpCode::MUL:
                emitInstruction("mul " + resultReg + ", " + leftReg + ", " + rightReg);
                break;
            case OpCode::DIV:
                emitInstruction("div " + resultReg + ", " + leftReg + ", " + rightReg);
                break;
            case OpCode::MOD:
                emitInstruction("rem " + resultReg + ", " + leftReg + ", " + rightReg);
                break;
            case OpCode::LT:
                emitInstruction("slt " + resultReg + ", " + leftReg + ", " + rightReg);
                break;
            case OpCode::GT:
                emitInstruction("slt " + resultReg + ", " + rightReg + ", " + leftReg);
                break;
            case OpCode::LE:
                emitInstruction("slt " + resultReg + ", " + rightReg + ", " + leftReg);
                emitInstruction("xori " + resultReg + ", " + resultReg + ", 1");
                break;
            case OpCode::GE:
                emitInstruction("slt " + resultReg + ", " + leftReg + ", " + rightReg);
                emitInstruction("xori " + resultReg + ", " + resultReg + ", 1");
                break;
            case OpCode::EQ:
                emitInstruction("xor " + resultReg + ", " + leftReg + ", " + rightReg);
                emitInstruction("seqz " + resultReg + ", " + resultReg);
                break;
            case OpCode::NE:
                emitInstruction("xor " + resultReg + ", " + leftReg + ", " + rightReg);
                emitInstruction("snez " + resultReg + ", " + resultReg);
                break;
            case OpCode::SHL:
                emitInstruction("sll " + resultReg + ", " + leftReg + ", " + rightReg);
                break;
            case OpCode::SHR:
                emitInstruction("sra " + resultReg + ", " + leftReg + ", " + rightReg);
                break;
            default:
                std::cerr << "错误: 未知的二元操作" << std::endl;
                break;
        }

        freeTempReg(rightReg);
        freeTempReg(leftReg);
    }

    storeRegister(resultReg, instr->result);
    freeTempReg(resultReg);
}

void CodeGenerator::processUnaryOp(const std::shared_ptr<UnaryOpInstr>& instr) {
    emitComment(instr->toString());
    
    std::string resultReg = allocTempReg();
    std::string operandReg = allocTempReg();

    loadOperand(instr->operand, operandReg);
    
    switch (instr->opcode) {
        case OpCode::NEG:
            emitInstruction("neg " + resultReg + ", " + operandReg);
            break;
        case OpCode::NOT:
            emitInstruction("seqz " + resultReg + ", " + operandReg);
            break;
        default:
            std::cerr << "错误: 未知的一元操作" << std::endl;
            break;
    }
    
    storeRegister(resultReg, instr->result);
    freeTempReg(operandReg);
    freeTempReg(resultReg);
}

void CodeGenerator::processAssign(const std::shared_ptr<AssignInstr>& instr) {
    emitComment(instr->toString());
    
    std::string reg = allocTempReg();
    loadOperand(instr->source, reg);
    storeRegister(reg, instr->target);
    freeTempReg(reg);
}

void CodeGenerator::processGoto(const std::shared_ptr<GotoInstr>& instr) {
    emitComment(instr->toString());
    emitInstruction("j " + instr->target->name);
}

void CodeGenerator::processIfGoto(const std::shared_ptr<IfGotoInstr>& instr) {
    emitComment(instr->toString());
    
    std::string condReg = allocTempReg();
    loadOperand(instr->condition, condReg);
    emitInstruction("bnez " + condReg + ", " + instr->target->name);
    freeTempReg(condReg);
}

void CodeGenerator::processParam(const std::shared_ptr<ParamInstr>& instr) {
    emitComment(instr->toString());
    paramQueue.push_back(instr->param);
}

void CodeGenerator::processCall(const std::shared_ptr<CallInstr>& instr) {
    if (!instr) {
        std::cerr << "错误: 空的函数调用指令" << std::endl;
        return;
    }

    emitComment(instr->toString());
    
    int paramCount = instr->paramCount;
    std::vector<std::shared_ptr<Operand>> params;
    
    if (!instr->params.empty()) {
        params = instr->params;
    } else if (!paramQueue.empty()) {
        if (paramQueue.size() >= paramCount) {
            size_t startIdx = paramQueue.size() - paramCount;
            params.reserve(paramCount);
            for (size_t i = 0; i < paramCount && (startIdx + i) < paramQueue.size(); ++i) {
                params.push_back(paramQueue[startIdx + i]);
            }
        } else {
            std::cerr << "错误: 参数队列大小不匹配, 预期 " << paramCount 
                      << ", 实际 " << paramQueue.size() << std::endl;
            return;
        }
    } else {
        std::cerr << "错误: 没有可用的参数" << std::endl;
        return;
    }

    analyzeUsedCallerSavedRegs();
    analyzeUsedCalleeSavedRegs();

    saveCallerSavedRegs();

    for (int i = 0; i < std::min(8, paramCount); ++i) {
        if (!params[i]) continue;
        loadOperand(params[i], "a" + std::to_string(i));
    }

    int stackParamOffset = 0;
    for (int i = 8; i < paramCount; ++i) {
        if (!params[i]) continue;
        std::string tempReg = allocTempReg();
        loadOperand(params[i], tempReg);
        emitInstruction("sw " + tempReg + ", " + std::to_string(stackParamOffset) + "(sp)");
        stackParamOffset += 4;
        freeTempReg(tempReg);
    }

    emitInstruction("call " + instr->funcName);
    restoreCallerSavedRegs();

    if (instr->result) {
        std::string resultReg = allocTempReg();
        emitInstruction("addi " + resultReg + ", a0" + ", 0");
        storeRegister(resultReg, instr->result);
        freeTempReg(resultReg);
    }

    if (!instr->params.empty() && paramCount > 0) {
        paramQueue.erase(paramQueue.end() - paramCount, paramQueue.end());
    }
}                                     

void CodeGenerator::processReturn(const std::shared_ptr<ReturnInstr>& instr) {
    emitComment(instr->toString());
    
    if (instr->value) {
        loadOperand(instr->value, "a0");
    } else if (currentFunctionReturnType != "void") {
        emitInstruction("li a0, 0");
    }
    
    emitInstruction("j " + currentFunction + "_epilogue");
}

void CodeGenerator::processLabel(const std::shared_ptr<LabelInstr>& instr) {
    emitLabel(instr->label);
}

void CodeGenerator::processFunctionBegin(const std::shared_ptr<FunctionBeginInstr>& instr) {
    if (!instr) {
        std::cerr << "错误: 空的函数开始指令" << std::endl;
        return;
    }
    
    currentFunction = instr->funcName;
    currentFunctionReturnType = instr->returnType;
    currentFunctionParams = instr->paramNames;

    frameSize = 8;
    localVarsSize = 0;
    calleeRegsSize = 0;
    localVars.clear();
    frameInitialized = false;

    analyzeUsedCalleeSavedRegs();
    
    for (size_t i = 0; i < currentFunctionParams.size(); i++) {
        if (i < 8) {
            regAlloc[currentFunctionParams[i]] = "a" + std::to_string(i);
        } else {
            localVars[currentFunctionParams[i]] = -12 - (i-8)*4;
            localVarsSize += 4;
        }
    }

    emitGlobal(instr->funcName);
    emitLabel(instr->funcName);
    emitPrologue(instr->funcName);

    if (currentFunctionParams.empty()) {
        return;
    }

    localVars.clear();

    emitComment("函数形参压栈");
    for (size_t i = 0; i < currentFunctionParams.size(); i++) {
        std::shared_ptr<Operand> paramVar = std::make_shared<Operand>(OperandType::VARIABLE, currentFunctionParams[i]);
        int offset = getOperandOffset(paramVar);
        
        if (i < 8) {
            std::string argReg = getArgRegister(i);
            regAlloc.erase(currentFunctionParams[i]);
            emitInstruction("sw " + argReg + ", " + std::to_string(offset) + "(fp)");
        } else {
            std::string tempReg = allocTempReg();
            int callerStackOffset = (i - 8) * 4 ;
            emitInstruction("lw " + tempReg + ", " + std::to_string(callerStackOffset) + "(fp)");
            emitInstruction("sw " + tempReg + ", " + std::to_string(offset) + "(fp)");
            freeTempReg(tempReg);
        }
    }
}

void CodeGenerator::processFunctionEnd(const std::shared_ptr<FunctionEndInstr>& instr) {
    emitLabel(currentFunction + "_epilogue");
    emitEpilogue(currentFunction);
    output << "\n";

    currentFunction = "";
    currentFunctionReturnType = "";
    currentFunctionParams.clear();
    regAlloc.clear();
    localVars.clear();
    frameInitialized = false;
}

// ==================== 输出辅助函数 ====================

std::string CodeGenerator::genLabel() {
    return "L" + std::to_string(labelCount++);
}

void CodeGenerator::emitComment(const std::string& comment) {
    output << "# " << comment << "\n";
}

void CodeGenerator::emitInstruction(const std::string& instr) {
    output << "\t" << instr << "\n";
}

void CodeGenerator::emitLabel(const std::string& label) {
    output << label << ":\n";
}

void CodeGenerator::emitGlobal(const std::string& name) {
    output << "\t.global " << name << "\n";
}

void CodeGenerator::emitSection(const std::string& section) {
    output << section << "\n";
}

// ==================== 函数序言和后记 ====================

void CodeGenerator::emitPrologue(const std::string& funcName) {
    emitComment("函数序言");
    
    resetStackOffset();

    calleeRegsSize = countUsedCalleeSavedRegs() * 4;
    callerRegsSize = countUsedCallerSavedRegs() * 4;
    int localsAndPadding = analyzeTempVars();
    int totalFrameSize = calleeRegsSize + callerRegsSize + localsAndPadding + 8;
    totalFrameSize = (totalFrameSize + 15) & ~15;
    frameSize = totalFrameSize;

    if (totalFrameSize <= 2048) {
        emitInstruction("addi sp, sp, -" + std::to_string(totalFrameSize));
    } else {
        emitInstruction("li t0, -" + std::to_string(totalFrameSize));
        emitInstruction("add sp, sp, t0");
    }

    if (totalFrameSize - 4 <= 2047) {
        emitInstruction("sw ra, " + std::to_string(totalFrameSize - 4) + "(sp)");
    } else {
        emitInstruction("li t0, " + std::to_string(totalFrameSize - 4));
        emitInstruction("add t0, sp, t0");
        emitInstruction("sw ra, 0(t0)");
    }
    
    if (totalFrameSize - 8 <= 2047) {
        emitInstruction("sw fp, " + std::to_string(totalFrameSize - 8) + "(sp)");
    } else {
        emitInstruction("li t0, " + std::to_string(totalFrameSize - 8));
        emitInstruction("add t0, sp, t0");
        emitInstruction("sw fp, 0(t0)");
    }

    if (totalFrameSize <= 2048) {
        emitInstruction("addi fp, sp, " + std::to_string(totalFrameSize));
    } else {
        emitInstruction("li t0, " + std::to_string(totalFrameSize));
        emitInstruction("add fp, sp, t0");
    }

    saveCalleeSavedRegs();
    frameInitialized = true;
    this->frameSize = totalFrameSize;
}

void CodeGenerator::emitEpilogue(const std::string& funcName) {
    emitComment("函数后记");
    
    restoreCalleeSavedRegs();
    
    if (frameSize - 8 <= 2047) {
        emitInstruction("lw fp, " + std::to_string(frameSize - 8) + "(sp)");
    } else {
        emitInstruction("li t0, " + std::to_string(frameSize - 8));
        emitInstruction("add t0, sp, t0");
        emitInstruction("lw fp, 0(t0)");
    }
    
    if (frameSize - 4 <= 2047) {
        emitInstruction("lw ra, " + std::to_string(frameSize - 4) + "(sp)");
    } else {
        emitInstruction("li t0, " + std::to_string(frameSize - 4));
        emitInstruction("add t0, sp, t0");
        emitInstruction("lw ra, 0(t0)");
    }
    
    if (frameSize <= 2048) {
        emitInstruction("addi sp, sp, " + std::to_string(frameSize));
    } else {
        emitInstruction("li t0, " + std::to_string(frameSize));
        emitInstruction("add sp, sp, t0");
    }

    emitInstruction("ret");
}

// ==================== 寄存器管理 ====================

void CodeGenerator::initializeRegisters() {
    registers = {
        {"zero", false, false, false, true, "常量0", false},
        {"t0", true, false, true, false, "临时寄存器0", false},
        {"t1", true, false, true, false, "临时寄存器1", false},
        {"t2", true, false, true, false, "临时寄存器2", false},
        {"t3", true, false, true, false, "临时寄存器3", false},
        {"t4", true, false, true, false, "临时寄存器4", false},
        {"t5", true, false, true, false, "临时寄存器5", false},
        {"t6", true, false, true, false, "临时寄存器6", false},
        {"s0", false, true, true, false, "保存寄存器0/帧指针", false},
        {"s1", false, true, true, false, "保存寄存器1", false},
        {"s2", false, true, true, false, "保存寄存器2", false},
        {"s3", false, true, true, false, "保存寄存器3", false},
        {"s4", false, true, true, false, "保存寄存器4", false},
        {"s5", false, true, true, false, "保存寄存器5", false},
        {"s6", false, true, true, false, "保存寄存器6", false},
        {"s7", false, true, true, false, "保存寄存器7", false},
        {"s8", false, true, true, false, "保存寄存器8", false},
        {"s9", false, true, true, false, "保存寄存器9", false},
        {"s10", false, true, true, false, "保存寄存器10", false},
        {"s11", false, true, true, false, "保存寄存器11", false},
        {"a0", true, false, true, false, "参数/返回值寄存器0", false},
        {"a1", true, false, true, false, "参数/返回值寄存器1", false},
        {"a2", true, false, true, false, "参数/返回值寄存器2", false},
        {"a3", true, false, true, false, "参数/返回值寄存器3", false},
        {"a4", true, false, true, false, "参数/返回值寄存器4", false},
        {"a5", true, false, true, false, "参数/返回值寄存器5", false},
        {"a6", true, false, true, false, "参数/返回值寄存器6", false},
        {"a7", true, false, true, false, "参数/返回值寄存器7", false},
        {"ra", true, false, false, true, "返回地址", false},
        {"sp", false, false, false, true, "栈指针", false},
        {"gp", false, false, false, true, "全局指针", false},
        {"tp", false, false, false, true, "线程指针", false},
        {"fp", false, true, false, true, "帧指针/s0", false}
    };
}

std::string CodeGenerator::allocTempReg() {
    if (nextTempReg >= tempRegs.size()) {
        nextTempReg = 0;
        for(Register& reg : registers) {
            reg.isUsed = false;
        }
    }

    for(Register& reg : registers) {
        if (reg.name == tempRegs[nextTempReg]) {
            if (reg.isUsed) {
                std::cerr << "错误: 临时寄存器 " << reg.name << " 已经被使用" << std::endl;
                return "";
            }
            reg.isUsed = true;
            usedCallerSavedRegs.insert(reg.name);
            break;
        }
    }
    nextTempReg++;
    return tempRegs[nextTempReg - 1];
}

void CodeGenerator::freeTempReg(const std::string& reg) {
}

// ==================== 寄存器保存/恢复 ====================

void CodeGenerator::saveCallerSavedRegs() {
    emitComment("保存调用者保存的寄存器");
    
    for (const auto& reg : usedCallerSavedRegs) {
        int offset = getRegisterStackOffset(reg);
        if (std::abs(offset) <= 2047) {
            emitInstruction("sw " + reg + ", " + std::to_string(offset) + "(fp)");
        } else {
            emitInstruction("li t0, " + std::to_string(offset));
            emitInstruction("add t0, fp, t0");
            emitInstruction("sw " + reg + ", 0(t0)");
        }
    }
}

void CodeGenerator::restoreCallerSavedRegs() {
    emitComment("恢复调用者保存的寄存器");
    
    for (const auto& reg : usedCallerSavedRegs) {
        int offset = getRegisterStackOffset(reg);
        if (std::abs(offset) <= 2047) {
            emitInstruction("lw " + reg + ", " + std::to_string(offset) + "(fp)");
        } else {
            emitInstruction("li t0, " + std::to_string(offset));
            emitInstruction("add t0, fp, t0");
            emitInstruction("lw " + reg + ", 0(t0)");
        }
    }
}

void CodeGenerator::saveCalleeSavedRegs() {
    emitComment("保存被调用者保存的寄存器");
    
    for (const auto& reg : usedCalleeSavedRegs) {
        int offset = getRegisterStackOffset(reg);
        if (std::abs(offset) <= 2047) {
            emitInstruction("sw " + reg + ", " + std::to_string(offset) + "(fp)");
        } else {
            emitInstruction("li t0, " + std::to_string(offset));
            emitInstruction("add t0, fp, t0");
            emitInstruction("sw " + reg + ", 0(t0)");
        }
    }
}

void CodeGenerator::restoreCalleeSavedRegs() {
    emitComment("恢复被调用者保存的寄存器");
    
    for (const auto& reg : usedCalleeSavedRegs) {
        int offset = getRegisterStackOffset(reg);
        if (std::abs(offset) <= 2047) {
            emitInstruction("lw " + reg + ", " + std::to_string(offset) + "(fp)");
        } else {
            emitInstruction("li t0, " + std::to_string(offset));
            emitInstruction("add t0, fp, t0");
            emitInstruction("lw " + reg + ", 0(t0)");
        }
    }
}

// ==================== 操作数加载/存储 ====================

void CodeGenerator::loadOperand(const std::shared_ptr<Operand>& op, const std::string& reg) {
    switch (op->type) {
        case OperandType::CONSTANT:
            emitInstruction("li " + reg + ", " + std::to_string(op->value));
            break;
            
        case OperandType::VARIABLE:
        case OperandType::TEMP:
            {
                auto it = regAlloc.find(op->name);
                if (it != regAlloc.end() && isValidRegister(it->second)) {
                    emitInstruction("addi " + reg + ", " + it->second + ", 0");
                } else {
                    int offset = getOperandOffset(op);
                    if (std::abs(offset) <= 2047) {
                        emitInstruction("lw " + reg + ", " + std::to_string(offset) + "(fp)");
                    } else {
                        std::string tempReg = (reg != "t0") ? "t0" : "t1";
                        emitInstruction("li " + tempReg + ", " + std::to_string(offset));
                        emitInstruction("add " + tempReg + ", fp, " + tempReg);
                        emitInstruction("lw " + reg + ", 0(" + tempReg + ")");
                    }
                }
            }
            break;
            
        case OperandType::LABEL:
            std::cerr << "警告: 尝试加载标签操作数" << std::endl;
            break;
            
        default:
            std::cerr << "错误: 未知的操作数类型" << std::endl;
            break;
    }
}

void CodeGenerator::storeRegister(const std::string& reg, const std::shared_ptr<Operand>& op) {
    if (op->type == OperandType::VARIABLE || op->type == OperandType::TEMP) {
        auto it = regAlloc.find(op->name);
        if (it != regAlloc.end() && isValidRegister(it->second)) {
            if (reg != it->second) {
                emitInstruction("addi " + it->second + ", " + reg + ", 0");
            }
        } else {
            int offset = getOperandOffset(op);
            if (std::abs(offset) <= 2047) {
                emitInstruction("sw " + reg + ", " + std::to_string(offset) + "(fp)");
            } else {
                std::string tempReg = (reg != "t0") ? "t0" : "t1";
                emitInstruction("li " + tempReg + ", " + std::to_string(offset));
                emitInstruction("add " + tempReg + ", fp, " + tempReg);
                emitInstruction("sw " + reg + ", 0(" + tempReg + ")");
            }
        }
    } else {
        std::cerr << "错误: 无法存储到非变量操作数" << std::endl;
    }
}

// ==================== 栈管理 ====================

void CodeGenerator::resetStackOffset() {
    regOffsetMap.clear();
    currentStackOffset = -12;
}

int CodeGenerator::getOperandOffset(const std::shared_ptr<Operand>& op) {
    if (!op) {
        std::cerr << "错误: 空操作数" << std::endl;
        return 0;
    }

    if (op->type != OperandType::VARIABLE && op->type != OperandType::TEMP) {
        std::cerr << "错误: 只有变量和临时变量有栈偏移" << std::endl;
        return 0;
    }
    
    auto it = localVars.find(op->name);
    if (it != localVars.end()) {
        return it->second;
    }
    
    for (size_t i = 0; i < currentFunctionParams.size(); i++) {
        if (currentFunctionParams[i] == op->name) {
            int offset = currentStackOffset;
            currentStackOffset -= 4;
            localVars[op->name] = offset;
            return offset;
        }
    }

    int offset = currentStackOffset;
    currentStackOffset -= 4;
    localVars[op->name] = offset;
    incrementLocalVarsSize(4);
    
    return offset;
}

int CodeGenerator::getRegisterStackOffset(const std::string& reg) {
    if (regOffsetMap.count(reg)) {
        return regOffsetMap[reg];
    }
        
    int offset = currentStackOffset;
    regOffsetMap[reg] = offset;
    currentStackOffset -= 4;
        
    return offset;
}

// ==================== 寄存器分析 ====================

void CodeGenerator::analyzeUsedCalleeSavedRegs() {
    usedCalleeSavedRegs.clear();
   
   for(Register& reg : registers) {
        if (reg.isCalleeSaved && reg.isUsed) {
            usedCalleeSavedRegs.insert(reg.name);
        }
    }
}

int CodeGenerator::countUsedCalleeSavedRegs() {
    int count = 0;

    const std::vector<std::string> calleeSavedRegs = {
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11"
    };

    for (const auto& instr : instructions) {
        std::string s = instr->toString();
        for (const auto& reg : calleeSavedRegs) {
            if (s.find(reg) != std::string::npos) {
                count++;
            }
        }
    }
    return count;
}

void CodeGenerator::analyzeUsedCallerSavedRegs() {
    usedCallerSavedRegs.clear();

    for(Register& reg : registers) {
        if (reg.isCallerSaved && reg.isUsed) {
            usedCallerSavedRegs.insert(reg.name);
        }
    }
}

int CodeGenerator::countUsedCallerSavedRegs() {
    int count = 0;

    const std::vector<std::string> callerSavedRegs = {
        "t0", "t1", "t2", "t3", "t4", "t5", "t6",
        "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
        "ra"
    };

    for (const auto& instr : instructions) {
        std::string s = instr->toString();
        for (const auto& reg : callerSavedRegs) {
            if (s.find(reg) != std::string::npos) {
                count++;
            }
        }
    }
    return count;
}

// ==================== 寄存器分配策略 ====================

void CodeGenerator::allocateRegisters() {
    switch (config.regAllocStrategy) {
        case RegisterAllocStrategy::LINEAR_SCAN:
            linearScanRegisterAllocation();
            break;
        case RegisterAllocStrategy::GRAPH_COLOR:
            graphColoringRegisterAllocation();
            break;
        default:
            break;
    }
}

void CodeGenerator::linearScanRegisterAllocation() {
    LinearScanRegisterAllocator allocator;
    
    std::vector<Register> allocatableRegs;
    for (const auto& reg : registers) {
        if (reg.isAllocatable && !reg.isReserved) {
            allocatableRegs.push_back(reg);
        }
    }
    
    regAlloc = allocator.allocate(instructions, allocatableRegs);
}

void CodeGenerator::graphColoringRegisterAllocation() {
    GraphColoringRegisterAllocator allocator;
    
    std::vector<Register> allocatableRegs;
    for (const auto& reg : registers) {
        if (reg.isAllocatable && !reg.isReserved) {
            allocatableRegs.push_back(reg);
        }
    }
    
    regAlloc = allocator.allocate(instructions, allocatableRegs);
}

// ==================== 优化函数 ====================

void CodeGenerator::optimizeStackLayout() {
    emitComment("栈布局优化开始");
    
    std::map<std::string, std::pair<int, int>> varLifetimes;
    analyzeVariableLifetimes(varLifetimes);
    
    std::vector<std::pair<std::string, std::pair<int, int>>> sortedVars(
        varLifetimes.begin(), varLifetimes.end());
    std::sort(sortedVars.begin(), sortedVars.end(), 
        [](const auto& a, const auto& b) {
            return a.second.second < b.second.second;
        });
    
    std::map<std::string, int> newOffsets;
    std::vector<std::pair<int, int>> allocatedRegions;
    
    for (const auto& [var, lifetime] : sortedVars) {
        int start = lifetime.first;
        int end = lifetime.second;
        int size = 4;
        
        bool found = false;
        for (auto& [offset, regionEnd] : allocatedRegions) {
            if (regionEnd < start) {
                newOffsets[var] = offset;
                regionEnd = end;
                found = true;
                break;
            }
        }
        
        if (!found) {
            int newOffset = allocatedRegions.empty() ? -4 : allocatedRegions.back().first - size;
            newOffsets[var] = newOffset;
            allocatedRegions.push_back({newOffset, end});
        }
    }
    
    localVars = newOffsets;
    
    emitComment("栈布局优化结束，新栈大小: " + std::to_string(frameSize));
}

int CodeGenerator::analyzeTempVars() {
    std::set<std::string> activeTemps;
    int maxTempSize = 0;
    
    for (const auto& instr : instructions) {
        auto defRegs = instr -> getDefRegisters();
        auto useRegs = instr -> getUseRegisters();
        
        for (const auto& reg : defRegs) {
            if (isTempReg(reg)) {
                activeTemps.erase(reg);
            }
        }
        
        for (const auto& reg : useRegs) {
            if (isTempReg(reg) && !isRegisterAllocated(reg)) {
                activeTemps.insert(reg);
            }
        }
        
        maxTempSize = std::max(maxTempSize, (int)activeTemps.size() * 4);
    }
    
    return maxTempSize;
}

void CodeGenerator::analyzeVariableLifetimes(std::map<std::string, std::pair<int, int>>& varLifetimes) {
    for (int i = 0; i < instructions.size(); i++) {
        auto instr = instructions[i];
        
        auto defined = IRAnalyzer::getDefinedVariables(instr);
        for (const auto& var : defined) {
            if (varLifetimes.find(var) == varLifetimes.end()) {
                varLifetimes[var] = {i, i};
            } else {
                varLifetimes[var].first = std::min(varLifetimes[var].first, i);
            }
        }
        
        auto used = IRAnalyzer::getUsedVariables(instr);
        for (const auto& var : used) {
            if (varLifetimes.find(var) == varLifetimes.end()) {
                varLifetimes[var] = {i, i};
            }
            varLifetimes[var].second = std::max(varLifetimes[var].second, i);
        }
    }
}

std::map<std::string, std::set<std::string>> CodeGenerator::buildInterferenceGraph() {
    std::map<std::string, std::set<std::string>> interferenceGraph;
    
    std::map<std::string, std::pair<int, int>> varLifetimes;
    analyzeVariableLifetimes(varLifetimes);
    
    for (const auto& [var, _] : varLifetimes) {
        interferenceGraph[var] = std::set<std::string>();
    }
    
    for (const auto& [var1, lifetime1] : varLifetimes) {
        for (const auto& [var2, lifetime2] : varLifetimes) {
            if (var1 != var2) {
                int overlapStart = std::max(lifetime1.first, lifetime2.first);
                int overlapEnd = std::min(lifetime1.second, lifetime2.second);
                
                if (overlapStart <= overlapEnd) {
                    interferenceGraph[var1].insert(var2);
                    interferenceGraph[var2].insert(var1);
                }
            }
        }
    }
    
    return interferenceGraph;
}

// ==================== 窥孔优化 ====================

void CodeGenerator::addPeepholePattern(const std::string& pattern, 
                                     std::function<bool(std::vector<std::string>&)> handler) {
    peepholePatterns[pattern] = handler;
}

void CodeGenerator::peepholeOptimize(std::vector<std::string>& instructions) {
    if (peepholePatterns.empty()) {
        addPeepholePattern("load_store_same", [](std::vector<std::string>& instrs) -> bool {
            if (instrs.size() < 2) return false;
            
            std::string load = instrs[0];
            std::string store = instrs[1];
            
            if (load.find("lw ") == 0 && store.find("sw ") == 0) {
                std::string loadReg = load.substr(3, load.find(",") - 3);
                std::string loadMem = load.substr(load.find(",") + 1);
                std::string storeReg = store.substr(3, store.find(",") - 3);
                std::string storeMem = store.substr(store.find(",") + 1);
                
                if (loadReg == storeReg && loadMem == storeMem) {
                    instrs.clear();
                    return true;
                }
            }
            return false;
        });
        
        addPeepholePattern("li_compare", [](std::vector<std::string>& instrs) -> bool {
            if (instrs.size() < 2) return false;
            
            std::string li = instrs[0];
            std::string branch = instrs[1];
            
            if (li.find("li ") == 0) {
                std::string reg = li.substr(3, li.find(",") - 3);
                std::string imm = li.substr(li.find(",") + 1);
                imm = imm.substr(imm.find_first_not_of(" \t"));
                
                if (imm == "0") {
                    if (branch.find("beq ") == 0) {
                        std::string branchParts = branch.substr(4);
                        size_t commaPos = branchParts.find(",");
                        std::string reg1 = branchParts.substr(0, commaPos);
                        reg1 = reg1.substr(reg1.find_first_not_of(" \t"));
                        
                        std::string rest = branchParts.substr(commaPos + 1);
                        size_t nextCommaPos = rest.find(",");
                        std::string reg2 = rest.substr(0, nextCommaPos);
                        reg2 = reg2.substr(reg2.find_first_not_of(" \t"));
                        
                        std::string label = rest.substr(nextCommaPos + 1);
                        label = label.substr(label.find_first_not_of(" \t"));
                        
                        if (reg2 == reg) {
                            instrs.clear();
                            instrs.push_back("beqz " + reg1 + ", " + label);
                            return true;
                        } else if (reg1 == reg) {
                            instrs.clear();
                            instrs.push_back("beqz " + reg2 + ", " + label);
                            return true;
                        }
                    }
                }
            }
            return false;
        });
        
        addPeepholePattern("redundant_move", [](std::vector<std::string>& instrs) -> bool {
            if (instrs.size() < 1) return false;
            
            std::string move = instrs[0];
            if (move.find("mv ") == 0) {
                std::string dst = move.substr(3, move.find(",") - 3);
                std::string src = move.substr(move.find(",") + 1);
                src = src.substr(src.find_first_not_of(" \t"));
                
                if (dst == src) {
                    instrs.clear();
                    return true;
                }
            }
            return false;
        });
    }
    
    bool changed = true;
    while (changed) {
        changed = false;
        
        for (size_t i = 0; i < instructions.size(); ) {
            bool patternApplied = false;
            
            for (auto& [pattern, handler] : peepholePatterns) {
                size_t windowSize = std::min(size_t(3), instructions.size() - i);
                std::vector<std::string> window(instructions.begin() + i, 
                                               instructions.begin() + i + windowSize);
                
                if (handler(window)) {
                    changed = true;
                    patternApplied = true;
                    
                    instructions.erase(instructions.begin() + i, 
                                      instructions.begin() + i + windowSize);
                    instructions.insert(instructions.begin() + i, 
                                       window.begin(), window.end());
                    
                    i += window.size();
                    break;
                }
            }
            
            if (!patternApplied) {
                i++;
            }
        }
    }
}

// ==================== 辅助函数 ====================

bool CodeGenerator::isValidRegister(const std::string& reg) const {
    for (const auto& r : registers) {
        if (r.name == reg) {
            return true;
        }
    }
    return false;
}

std::string CodeGenerator::getArgRegister(int paramIndex) const {
    if (paramIndex >= 0 && paramIndex < argRegs.size()) {
        return argRegs[paramIndex];
    }
    std::cerr << "错误: 参数索引超出范围" << std::endl;
    return "a0";
}

// ==================== 寄存器分配器实现 ====================

std::map<std::string, std::string> NaiveRegisterAllocator::allocate(
    const std::vector<std::shared_ptr<IRInstr>>& instructions,
    const std::vector<Register>& availableRegs) {
    std::map<std::string, std::string> allocation;
    
    std::set<std::string> variables;
    for (const auto& instr : instructions) {
        auto defined = IRAnalyzer::getDefinedVariables(instr);
        for (const auto& var : defined) {
            variables.insert(var);
        }
        
        auto used = IRAnalyzer::getUsedVariables(instr);
        for (const auto& var : used) {
            variables.insert(var);
        }
    }
    
    std::vector<std::string> allocatableRegs;
    for (const auto& reg : availableRegs) {
        if (reg.isAllocatable && reg.isCalleeSaved) {
            allocatableRegs.push_back(reg.name);
        }
    }
    
    if (allocatableRegs.size() < variables.size()) {
        for (const auto& reg : availableRegs) {
            if (reg.isAllocatable && reg.isCallerSaved && 
                std::find(allocatableRegs.begin(), allocatableRegs.end(), reg.name) == allocatableRegs.end()) {
                allocatableRegs.push_back(reg.name);
            }
        }
    }
    
    auto regIt = allocatableRegs.begin();
    for (const auto& var : variables) {
        if (regIt != allocatableRegs.end()) {
            allocation[var] = *regIt;
            ++regIt;
        } else {
            break;
        }
    }
    
    return allocation;
}

std::map<std::string, std::string> LinearScanRegisterAllocator::allocate(
    const std::vector<std::shared_ptr<IRInstr>>& instructions,
    const std::vector<Register>& availableRegs) {
    
    std::map<std::string, std::string> allocation;
    
    std::vector<LiveInterval> intervals = computeLiveIntervals(instructions);
    
    // Prioritize variables that are likely loop counters/accumulators
    auto isLoopVariable = [](const std::string& var) {
        return var.find("i") == 0 || var.find("j") == 0 || var.find("k") == 0 || 
               var.find("result") != std::string::npos || var.find("acc") != std::string::npos ||
               var.find("t") == 0; // temp variables
    };
    
    // Sort intervals, prioritizing loop variables
    std::sort(intervals.begin(), intervals.end(), [&](const LiveInterval& a, const LiveInterval& b) {
        bool aIsLoop = isLoopVariable(a.var);
        bool bIsLoop = isLoopVariable(b.var);
        if (aIsLoop != bIsLoop) return aIsLoop > bIsLoop;
        return a.start < b.start;
    });
    
    std::vector<std::string> freeRegs;
    for (const auto& reg : availableRegs) {
        if (reg.isCalleeSaved && reg.name != "fp" && reg.name != "s0") {
            freeRegs.push_back(reg.name);
        }
    }
    
    std::map<std::string, std::pair<LiveInterval, std::string>> active;
    
    for (const auto& interval : intervals) {
        std::vector<std::string> expired;
        for (auto& [var, pair] : active) {
            if (pair.first.end < interval.start) {
                freeRegs.push_back(pair.second);
                expired.push_back(var);
            }
        }
        
        for (const auto& var : expired) {
            active.erase(var);
        }
        
        if (freeRegs.empty()) {
            // For loop variables, try harder to find a register
            if (isLoopVariable(interval.var)) {
                // Spill a non-loop variable if possible
                std::string victimVar;
                int latestEnd = -1;
                
                for (const auto& [var, pair] : active) {
                    if (!isLoopVariable(var) && pair.first.end > latestEnd) {
                        latestEnd = pair.first.end;
                        victimVar = var;
                    }
                }
                
                if (!victimVar.empty()) {
                    allocation[interval.var] = active[victimVar].second;
                    freeRegs.push_back(active[victimVar].second);
                    active.erase(victimVar);
                    active[interval.var] = {interval, allocation[interval.var]};
                    continue;
                }
            }
            
            // Standard spilling
            std::string victimVar;
            int latestEnd = -1;
            
            for (const auto& [var, pair] : active) {
                if (pair.first.end > latestEnd) {
                    latestEnd = pair.first.end;
                    victimVar = var;
                }
            }
            
            if (latestEnd > interval.end && !victimVar.empty()) {
                allocation[interval.var] = active[victimVar].second;
                freeRegs.push_back(active[victimVar].second);
                active.erase(victimVar);
                
                active[interval.var] = {interval, allocation[interval.var]};
            }
        } else {
            std::string reg = freeRegs.back();
            freeRegs.pop_back();
            
            allocation[interval.var] = reg;
            active[interval.var] = {interval, reg};
        }
    }
    
    return allocation;
}

std::vector<LinearScanRegisterAllocator::LiveInterval> LinearScanRegisterAllocator::computeLiveIntervals(
    const std::vector<std::shared_ptr<IRInstr>>& instructions) {
    
    std::map<std::string, LiveInterval> intervalMap;
    
    for (int i = 0; i < instructions.size(); i++) {
        auto instr = instructions[i];
        
        auto defined = IRAnalyzer::getDefinedVariables(instr);
        for (const auto& var : defined) {
            if (intervalMap.find(var) == intervalMap.end()) {
                intervalMap[var] = {var, i, i};
            } else {
                intervalMap[var].start = std::min(intervalMap[var].start, i);
            }
        }
        
        auto used = IRAnalyzer::getUsedVariables(instr);
        for (const auto& var : used) {
            if (intervalMap.find(var) == intervalMap.end()) {
                intervalMap[var] = {var, i, i};
            }
            intervalMap[var].end = std::max(intervalMap[var].end, i);
        }
    }
    
    std::vector<LiveInterval> intervals;
    for (const auto& [var, interval] : intervalMap) {
        intervals.push_back(interval);
    }
    
    return intervals;
}

std::map<std::string, std::string> GraphColoringRegisterAllocator::allocate(
    const std::vector<std::shared_ptr<IRInstr>>& instructions,
    const std::vector<Register>& availableRegs) {
    
    std::map<std::string, std::string> allocation;
    
    auto interferenceGraph = buildInterferenceGraph(instructions);
    
    if (interferenceGraph.empty()) {
        return allocation;
    }
    
    auto simplifiedOrder = simplify(interferenceGraph);
    allocation = color(simplifiedOrder, interferenceGraph, availableRegs);
    
    return allocation;
}

std::map<std::string, std::set<std::string>> GraphColoringRegisterAllocator::buildInterferenceGraph(
    const std::vector<std::shared_ptr<IRInstr>>& instructions) {
    
    std::map<std::string, std::set<std::string>> graph;
    
    std::map<std::string, std::pair<int, int>> varLifetimes;
    
    for (int i = 0; i < instructions.size(); i++) {
        auto instr = instructions[i];
        
        auto defined = IRAnalyzer::getDefinedVariables(instr);
        for (const auto& var : defined) {
            if (varLifetimes.find(var) == varLifetimes.end()) {
                varLifetimes[var] = {i, i};
            } else {
                varLifetimes[var].first = std::min(varLifetimes[var].first, i);
            }
        }
        
        auto used = IRAnalyzer::getUsedVariables(instr);
        for (const auto& var : used) {
            if (varLifetimes.find(var) == varLifetimes.end()) {
                varLifetimes[var] = {i, i};
            }
            varLifetimes[var].second = std::max(varLifetimes[var].second, i);
        }
    }
    
    for (const auto& [var, _] : varLifetimes) {
        graph[var] = std::set<std::string>();
    }
    
    for (const auto& [var1, lifetime1] : varLifetimes) {
        for (const auto& [var2, lifetime2] : varLifetimes) {
            if (var1 != var2) {
                int overlapStart = std::max(lifetime1.first, lifetime2.first);
                int overlapEnd = std::min(lifetime1.second, lifetime2.second);
                
                if (overlapStart <= overlapEnd) {
                    graph[var1].insert(var2);
                    graph[var2].insert(var1);
                }
            }
        }
    }
    
    return graph;
}

std::vector<std::string> GraphColoringRegisterAllocator::simplify(
    std::map<std::string, std::set<std::string>>& graph) {
    
    std::vector<std::string> simplifiedOrder;
    
    auto workGraph = graph;
    
    while (!workGraph.empty()) {
        std::string nodeToRemove;
        int minDegree = std::numeric_limits<int>::max();
        
        for (const auto& [node, neighbors] : workGraph) {
            if (neighbors.size() < minDegree) {
                minDegree = neighbors.size();
                nodeToRemove = node;
            }
        }
        
        if (nodeToRemove.empty()) {
            int maxDegree = -1;
            for (const auto& [node, neighbors] : workGraph) {
                if (neighbors.size() > maxDegree) {
                    maxDegree = neighbors.size();
                    nodeToRemove = node;
                }
            }
        }
        
        for (auto& [_, neighbors] : workGraph) {
            neighbors.erase(nodeToRemove);
        }
        workGraph.erase(nodeToRemove);
        
        simplifiedOrder.push_back(nodeToRemove);
    }
    
    std::reverse(simplifiedOrder.begin(), simplifiedOrder.end());
    return simplifiedOrder;
}

std::map<std::string, std::string> GraphColoringRegisterAllocator::color(
    const std::vector<std::string>& simplifiedOrder,
    const std::map<std::string, std::set<std::string>>& originalGraph,
    const std::vector<Register>& availableRegs) {
    
    std::map<std::string, std::string> allocation;
    
    std::vector<std::string> regNames;
    for (const auto& reg : availableRegs) {
        if (reg.isAllocatable && !reg.isReserved) {
            regNames.push_back(reg.name);
        }
    }
    
    for (const auto& var : simplifiedOrder) {
        auto it = originalGraph.find(var);
        if (it == originalGraph.end()) continue;
        
        const auto& neighbors = it->second;
        
        std::set<std::string> usedColors;
        for (const auto& neighbor : neighbors) {
            auto allocIt = allocation.find(neighbor);
            if (allocIt != allocation.end()) {
                usedColors.insert(allocIt->second);
            }
        }
        
        std::string selectedReg;
        for (const auto& reg : regNames) {
            if (usedColors.find(reg) == usedColors.end()) {
                selectedReg = reg;
                break;
            }
        }
        
        if (!selectedReg.empty()) {
            allocation[var] = selectedReg;
        }
    }
    
    return allocation;
}