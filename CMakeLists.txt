cmake_minimum_required(VERSION 4.2) #min version

project(nexcore) #project name

set(CMAKE_CXX_STANDARD 20) #standard c++ version

add_executable(nexc src/main.cpp
        src/tokenization.h
        src/parser.h) #what to execute