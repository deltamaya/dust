cmake_minimum_required(VERSION 3.27)
project(dust)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_PREFIX_PATH "E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\lib\\cmake")
include_directories("include/"
        "E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\include")


link_directories("E:\\clang+llvm-18.1.0-x86_64-pc-windows-msvc\\lib")

find_package(LLVM 18.1.0 REQUIRED)
llvm_map_components_to_libnames(
        LLVM_LIBRARIES
        Core
        Support
)
add_executable(dust src/main.cpp
        src/lexer/lexer.cc
        src/parser/parser.cc
        src/parser/handler.cc
)
target_link_options(dust PRIVATE "/MD")
target_compile_options(dust PRIVATE "/MD")
target_include_directories(dust PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(dust PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(dust PRIVATE ${LLVM_LIBRARIES})
