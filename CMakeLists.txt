cmake_minimum_required(VERSION 3.10)
project(toycc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O2 -Wall)

add_executable(your_compiler
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
)
