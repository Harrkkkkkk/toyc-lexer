cmake_minimum_required(VERSION 3.16)
project(ToyC_Compiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# 包含目录
include_directories(src)
include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Flex/Bison generation
FLEX_TARGET(Lexer lexer/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
BISON_TARGET(Parser parser/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

# 源文件 - 使用 Flex/Bison
set(SOURCES
    main.cpp
    ${FLEX_Lexer_OUTPUTS}
    ${BISON_Parser_OUTPUTS}
    parser/ast.cpp
    semantic/semantic.cpp
    ir/irgen.cpp
    codegen/codegen.cpp
)

# 创建可执行文件
add_executable(toyc_compiler ${SOURCES})

# 编译选项
target_compile_options(toyc_compiler PRIVATE -Wall -Wextra -O2)

# 创建优化版本的编译器（用于-opt参数）
add_executable(toyc_compiler_opt ${SOURCES})
target_compile_definitions(toyc_compiler_opt PRIVATE ENABLE_OPTIMIZATION=1)
target_compile_options(toyc_compiler_opt PRIVATE -Wall -Wextra -O2)