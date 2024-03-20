cmake_minimum_required(VERSION 3.27)
project(dust)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_PREFIX_PATH "E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\lib\\cmake")
include_directories("include/"
        "E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\include")


link_directories("E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\lib")

find_package(LLVM 18.1.0 REQUIRED)

add_executable("dust" src/main.cpp
        src/lexer/lexer.cc
        src/parser/parser.cc
        src/parser/handler.cc
        src/parser/initializer.cc
        include/jit/dustjit.h
        src/ast/ast.cc
        lib/print.cc
        src/parser/utils.cc
)

execute_process(COMMAND E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\bin\\llvm-config.exe --libs all
        OUTPUT_VARIABLE llvm_libraries)
string(STRIP ${llvm_libraries} llvm_clean)
string(REPLACE "E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\lib\\" "" llvm_clean ${llvm_clean})
#message(NOTICE ${llvm_clean})
#message(NOTICE ${llvm_libraries})
target_compile_options(dust PRIVATE "/MD")
target_include_directories(dust PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(dust PRIVATE ${llvm_clean} Ws2_32.lib)
#add_library(print lib/print.cc)
