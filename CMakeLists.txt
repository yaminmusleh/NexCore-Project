cmake_minimum_required(VERSION 4.2)

project(nexcore)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
        src
        generated
)

add_executable(nexc
        src/main.cpp
        src/parser_support.cpp

        src/arena.h
        src/ast.h
        src/generation.h
        src/parser_support.h

        generated/Parser.cpp
        generated/Parser.h
        generated/Scanner.cpp
        generated/Scanner.h
)