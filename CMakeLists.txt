cmake_minimum_required(VERSION 3.20)
project(ToyC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(src)
include_directories(.)

# 源文件
set(SOURCES
    src/toyc_main.cpp
    src/ast.cpp
    src/symbol_table.cpp
    src/semantic.cpp
    src/codegen.cpp
    src/optimizer.cpp
    src/lexer.cpp
    src/parser.cpp
)

# 创建可执行文件
add_executable(toyc_compiler ${SOURCES})

# 编译选项
target_compile_options(toyc_compiler PRIVATE -Wall -Wextra -O2)

# 创建优化版本的编译器（用于-opt参数）
add_executable(toyc_compiler_opt ${SOURCES})
target_compile_definitions(toyc_compiler_opt PRIVATE ENABLE_OPTIMIZATION=1)
target_compile_options(toyc_compiler_opt PRIVATE -Wall -Wextra -O2)
