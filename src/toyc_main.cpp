#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <cstring>

#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
#include "optimizer.h"

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " [-opt] < input.tc > output.s\n";
    std::cerr << "Options:\n";
    std::cerr << "  -opt    Enable optimizations\n";
    std::cerr << "\nExample:\n";
    std::cerr << "  " << program_name << " < test.tc > test.s\n";
    std::cerr << "  " << program_name << " -opt < test.tc > test.s\n";
}

std::string read_stdin() {
    std::ostringstream buffer;
    std::string line;
    
    while (std::getline(std::cin, line)) {
        buffer << line << "\n";
    }
    
    return buffer.str();
}

int main(int argc, char* argv[]) {
    bool enable_optimization = false;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-opt") == 0) {
            enable_optimization = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Error: Unknown option '" << argv[i] << "'\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    try {
        // 1. 读取输入
        std::string source_code = read_stdin();
        if (source_code.empty()) {
            std::cerr << "Error: No input provided\n";
            return 1;
        }
        
        #ifdef DEBUG_INPUT
        std::cerr << "=== Input Source Code ===\n" << source_code << "\n";
        #endif
        
        // 2. 词法分析
        Lexer lexer(source_code);
        auto tokens = lexer.tokenize();
        
        #ifdef DEBUG_TOKENS
        std::cerr << "=== Tokens ===\n";
        for (const auto& token : tokens) {
            std::cerr << Lexer::token_type_to_string(token.type) 
                      << ": " << token.value << "\n";
        }
        #endif
        
        // 3. 语法分析
        Parser parser(tokens);
        auto ast = parser.parse();
        
        if (parser.has_error || !ast) {
            std::cerr << "Error: Syntax analysis failed\n";
            if (parser.has_error) {
                std::cerr << parser.error_message << std::endl;
            }
            return 1;
        }
        
        #ifdef DEBUG_AST
        std::cerr << "=== Abstract Syntax Tree ===\n";
        ast->print();
        std::cerr << "============================\n";
        #endif
        
        // 4. 语义分析
        SemanticAnalyzer semantic_analyzer;
        if (!semantic_analyzer.analyze(*ast)) {
            std::cerr << "Error: Semantic analysis failed\n";
            semantic_analyzer.print_errors();
            return 1;
        }
        
        #ifdef DEBUG_SEMANTIC
        std::cerr << "=== Semantic Analysis Passed ===\n";
        #endif
        
        // 5. 优化
        if (enable_optimization) {
            Optimizer optimizer(true);
            optimizer.optimize(*ast);
            
            #ifdef DEBUG_OPTIMIZATION
            std::cerr << "=== Optimization Applied: " 
                      << optimizer.get_optimizations_count() << " optimizations ===\n";
            #endif
        }
        
        // 6. 代码生成
        CodeGenerator code_generator(&semantic_analyzer.get_symbol_table());
        std::string assembly_code = code_generator.generate(*ast);
        
        // 7. 输出汇编代码
        std::cout << assembly_code;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error: Unknown exception occurred" << std::endl;
        return 1;
    }
}
