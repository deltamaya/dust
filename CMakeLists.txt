cmake_minimum_required(VERSION 3.27)
project(dust)

set(CMAKE_CXX_STANDARD 23)

include_directories("include/"
        "E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\include")
link_directories("E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\lib")

add_executable(dust src/main.cpp
        src/lexer/lexer.cc
        include/lexer/lexer.h
        include/minilog.h
        src/parser/parser.cc
        include/ast/ast.h
        include/parser/parser.h)

#target_link_libraries(dust )