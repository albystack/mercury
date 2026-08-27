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
