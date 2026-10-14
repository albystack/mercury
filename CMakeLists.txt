cmake_minimum_required(VERSION 3.25)
project(Mercury VERSION 0.1.0 DESCRIPTION "Queue-aware market making engine" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(MERCURY_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

find_package(Boost CONFIG REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
include(FetchContent)
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# An interface target shares build requirements without compiling a source file.
add_library(mercury_options INTERFACE)
target_include_directories(mercury_options INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_compile_options(mercury_options INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
if(MERCURY_ENABLE_SANITIZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "Sanitizers require Clang or GCC")
  endif()
  target_compile_options(mercury_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(mercury_options INTERFACE -fsanitize=address,undefined)
endif()

add_library(mercury_feed
  src/feed/coinbase_websocket_client.cpp
  src/feed/conversion.cpp
  src/feed/coinbase_parser.cpp
)
target_link_libraries(mercury_feed PUBLIC mercury_options Boost::system OpenSSL::SSL OpenSSL::Crypto
  PRIVATE nlohmann_json::nlohmann_json)
add_library(mercury_data src/data/recording.cpp)
target_link_libraries(mercury_data PUBLIC mercury_feed PRIVATE nlohmann_json::nlohmann_json)

function(mercury_app name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE mercury_data)
endfunction()
mercury_app(mercury apps/mercury.cpp)
mercury_app(mercury_feed_smoke apps/feed_smoke.cpp)
mercury_app(mercury_live apps/live_book.cpp)
mercury_app(mercury_record apps/record_feed.cpp)
mercury_app(mercury_replay apps/replay_feed.cpp)
mercury_app(mercury_backtest apps/backtest.cpp)
mercury_app(mercury_benchmark apps/benchmark.cpp)

include(CTest)
if(BUILD_TESTING)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(googletest)
  add_executable(mercury_tests
    tests/market/types_test.cpp
    tests/book/order_book_test.cpp
    tests/market/events_test.cpp
    tests/market/market_data_processor_test.cpp
    tests/feed/parser_test.cpp
    tests/feed/session_test.cpp
    tests/data/recording_test.cpp
    tests/data/replay_test.cpp
    tests/execution/orders_test.cpp
    tests/simulation/scheduler_test.cpp
    tests/simulation/exchange_test.cpp
    tests/simulation/paper_engine_test.cpp
    tests/strategy/quotes_test.cpp
    tests/risk/risk_test.cpp
    tests/metrics/latency_test.cpp
  )
  target_link_libraries(mercury_tests PRIVATE mercury_data GTest::gtest_main nlohmann_json::nlohmann_json)
  target_compile_definitions(mercury_tests PRIVATE MERCURY_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures")
  include(GoogleTest)
  gtest_discover_tests(mercury_tests)
  add_test(NAME ReplayApp COMMAND mercury_replay ${PROJECT_SOURCE_DIR}/tests/fixtures/sample.jsonl)
  set_tests_properties(ReplayApp PROPERTIES PASS_REGULAR_EXPRESSION "records=8 updates=3 trades=3 gaps=0")
  add_test(NAME BacktestApp COMMAND mercury_backtest ${PROJECT_SOURCE_DIR}/tests/fixtures/sample.jsonl)
  set_tests_properties(BacktestApp PROPERTIES PASS_REGULAR_EXPRESSION "orders=4 fills=3")
endif()
