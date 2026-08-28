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


# TESTING 
# FetchContent lets CMake download third-party dependencies as part of the configuration process for the project 
# This means a developer clonign Mercury need not manually install GoogleTest before building the test suite 
include(FetchContent)

# GoogleTest is the unit testing framework we will use 
# We pin a specific version rather than the latest branch. 
# reproducible builds matter. 
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git 
  GIT_TAG v1.17.0
  GIT_SHALLOW TRUE 
)

# Download GoogleTest if necessary and make its CMake targets available 
FetchContent_MakeAvailable(googletest)

# Tell CMake that this project contains tests 
enable_testing()

#Create a seperate executable containing Mercury's unit tests 
# This is NOT the normal Mercury application 
# We will eventually have: ./build/mercury for the application, and a seperate testing exec managed through CTest 
add_executable(
  mercury_tests
  tests/market/types_test.cpp
)

# Our tests need access to Mercury's public headers 
# THis is the same include dir used by the normal Mercury executable 
target_include_directories(
  mercury_tests
  PRIVATE
      ${PROJECT_SOURCE_DIR}/include
)

# Link our test executable against GoogleTest 
# GTest::gtest_main provides: 
# - the google test framework 
# a main() function for running the tests 
# Therefore we do not need to write main() ourselves inside the test files 
target_link_libraries(
  mercury_tests
  PRIVATE 
    GTest::gtest_main
)

# Give test code the same strict warning settings as main app 
# We do not want sloppy code to hide inside the tests 

target_compile_options(
  mercury_tests
  PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

#GoogleTest integrates with CTest through this helper module 
include(GoogleTest)

# Discover each TEST(...) automatically and register it with CTest
gtest_discover_tests(mercury_tests)
