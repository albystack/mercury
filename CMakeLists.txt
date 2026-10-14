cmake_minimum_required(VERSION 3.25)

project(
    Mercury
    VERSION 0.1.0
    DESCRIPTION "Queue-aware market making engine"
    LANGUAGES CXX
)

find_package(Boost CONFIG REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

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

include(FetchContent)
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(mercury_feed
  src/feed/coinbase_websocket_client.cpp
  src/feed/conversion.cpp
  src/feed/coinbase_parser.cpp
)
target_include_directories(mercury_feed PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mercury_feed PUBLIC Boost::system OpenSSL::SSL OpenSSL::Crypto
  PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(mercury_feed PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_executable(mercury_feed_smoke apps/feed_smoke.cpp)
target_link_libraries(mercury_feed_smoke PRIVATE mercury_feed)

target_include_directories(
  mercury_feed_smoke
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(
  mercury_feed_smoke
  PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

target_link_libraries(
  mercury_feed_smoke
  PRIVATE
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
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
  tests/book/order_book_test.cpp
  tests/market/events_test.cpp
  tests/market/market_data_processor_test.cpp
  tests/risk/risk_test.cpp
  tests/strategy/quotes_test.cpp
  tests/simulation/exchange_test.cpp
  tests/simulation/scheduler_test.cpp
  tests/execution/orders_test.cpp
  tests/data/replay_test.cpp
  tests/data/recording_test.cpp
  tests/feed/session_test.cpp
  tests/feed/parser_test.cpp
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
    nlohmann_json::nlohmann_json
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

function(mercury_app name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE mercury_data)
  target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endfunction()
mercury_app(mercury_live apps/live_book.cpp)

add_library(mercury_data src/data/recording.cpp)
target_include_directories(mercury_data PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mercury_data PUBLIC mercury_feed PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(mercury_data PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
mercury_app(mercury_record apps/record_feed.cpp)
target_link_libraries(mercury_tests PRIVATE mercury_data)

mercury_app(mercury_replay apps/replay_feed.cpp)
target_compile_definitions(mercury_tests PRIVATE MERCURY_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures")
