cmake_minimum_required(VERSION 3.21)

project(nexcore-studio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(nexcore-studio
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(nexcore-studio PRIVATE Qt6::Widgets)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(nexcore-studio PRIVATE -Wall -Wextra)
endif()