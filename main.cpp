// main.cpp - 编译器主程序
#include "parser/ast.h"
#include "semantic/semantic.h"
#include "ir/ir.h"
#include "ir/irgen.h"
#include "codegen/codegen.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>

// Declare external parser function and root
extern int yyparse();
extern std::shared_ptr<CompUnit> root;
extern FILE* yyin;

int main(int argc, char* argv[]) {
    bool enableOptimization = false;
    bool enablePrintIR = false;
    
    std::string filename;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-opt") {
            enableOptimization = true;
            std::cerr << "Optimization enabled." << std::endl;
        } else {
            filename = arg;
        }
    }
    
    if (!filename.empty()) {
        yyin = fopen(filename.c_str(), "r");
        if (!yyin) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return 1;
        }
    } else {
        yyin = stdin;
    }
    
    if (yyparse() != 0) {
        std::cerr << "Error: Parsing failed." << std::endl;
        return 1;
    }
    
    if (!root) {
        std::cerr << "Error: Parsing failed (no AST generated)." << std::endl;
        return 1;
    }
    
    SemanticAnalyzer semanticAnalyzer;
    if (!semanticAnalyzer.analyze(root)) {
        std::cerr << "Error: Semantic analysis failed." << std::endl;
        return 1;
    }

    IRGenConfig irConfig;
    if (enableOptimization) {
        irConfig.enableOptimizations = true;
    }
    
    IRGenerator irGenerator(irConfig);
    irGenerator.generate(root);
    
    if (enablePrintIR) {
        IRPrinter::print(irGenerator.getInstructions(), std::cerr);
    }
    
    CodeGenConfig config;
    if (enableOptimization) {
        config.regAllocStrategy = RegisterAllocStrategy::LINEAR_SCAN;
        config.optimizeStackLayout = true;
        config.eliminateDeadStores = true;
        config.enablePeepholeOptimizations = true;
    }
    
    std::stringstream outputStream;
    
    CodeGenerator generator(outputStream, irGenerator.getInstructions(), config);
    generator.generate();
    
    std::cout << outputStream.str();
    
    return 0;
}