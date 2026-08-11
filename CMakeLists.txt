cmake_minimum_required(VERSION 4.2)

project(nexcore)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(nexc
        src/main.cpp
        src/tokenization.h
        src/arena.h
        src/ast.h
        src/parser.h
        src/parser.cpp
        src/generation.h
)