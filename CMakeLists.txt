cmake_minimum_required(VERSION 3.27)
project(dust)

set(CMAKE_CXX_STANDARD 23)

add_executable(dust src/main.cpp
        src/lexer/lexer.cc)
