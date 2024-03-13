cmake_minimum_required(VERSION 3.27)
project(dust)

set(CMAKE_CXX_STANDARD 23)

include_directories("include/")

add_executable(dust src/main.cpp
        src/lexer/lexer.cc
        include/lexer/lexer.h
        include/minilog.h
        src/parser/parser.cc
        include/ast/ast.h)
