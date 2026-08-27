cmake_minimum_required(VERSION 3.25)

project(
    Mercury
    VERSION 0.1.0
    DESCRIPTION "Queue-aware market making engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add CMake to generate compile_commands.json 
# this file records the exact compiler command used for every c++ source file, including 
# - the c++ standard 
# - include directories 
# - warning flags 
# - compiler options 
# clangd (the c++ language server used by neovim) can read this file so the editor understands the project is exactly the same as the compiler
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    mercury
    apps/mercury.cpp
)

target_include_directories(
    mercury
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(
    mercury
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
)
