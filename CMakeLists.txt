cmake_minimum_required(VERSION 4.0)
project(MarketMakerSimulator CXX)
enable_testing()
include(CTest)
include(GoogleTest)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
    message("*** Build type not set.  defaulting to Debug")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options( 
        -g3 
        -Og
        -fsanitize=address,undefined,leak
        -fno-omit-frame-pointer 
        -Wall
        -Wno-interference-size
    )
else()
    add_compile_options(
        -O3
        -g
        -march=native
        -Wall
    )
endif()
add_link_options(-Wno-interference-size -fsanitize=address,undefined,leak)


# Compiler flags and other goodies
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(nlohmann_json REQUIRED)
# find_package(toml11 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(GTest REQUIRED)
include(FetchContent)

FetchContent_Declare(
  quicklib
  GIT_REPOSITORY https://github.com/xbazzi/quicklib.git
  GIT_TAG        master

  # Local for testing
  # SOURCE_DIR       /path/to/local/quicklib
  # SOURCE_DIR       /home/xbazzi/repos/quicklib
)

# Disable building tests and benchmarks for quicklib dependency
set(BUILD_TESTING OFF)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(SKIP_BENCHMARKS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(quicklib)
set(QUICKLIB_TCP_HEADER "${quicklib_SOURCE_DIR}/include/quick/io/Tcp.hh")
if(EXISTS "${QUICKLIB_TCP_HEADER}")
    string(CONCAT LEGACY_ERROR_INCLUDE "fi" "ah/error/Error.hh")
    file(READ "${QUICKLIB_TCP_HEADER}" QUICKLIB_TCP_HEADER_CONTENT)
    string(REPLACE "${LEGACY_ERROR_INCLUDE}" "mms/error/Error.hh" QUICKLIB_TCP_HEADER_CONTENT "${QUICKLIB_TCP_HEADER_CONTENT}")
    file(WRITE "${QUICKLIB_TCP_HEADER}" "${QUICKLIB_TCP_HEADER_CONTENT}")
endif()
set(BUILD_TESTING ON)


include_directories("${CMAKE_SOURCE_DIR}/include")

# Put all executables in the same directory
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Convention for executables and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

message("************************************************************")
message("**** Building MarketMakerSimulator in ${CMAKE_BUILD_TYPE} mode ")
message("************************************************************")


# Add `options` project
add_subdirectory(src/mms/options)

# Common (non-main) sources
file(GLOB_RECURSE COMMON_SRC
    "${CMAKE_SOURCE_DIR}/src/mms/*.cc"
    "${CMAKE_SOURCE_DIR}/src/mms/options/*.cc"
)

# Exclude mains from the common sources
list(REMOVE_ITEM COMMON_SRC 
    "${CMAKE_SOURCE_DIR}/src/mms/options/main.cc"
    "${CMAKE_SOURCE_DIR}/src/mms/client/main.cc"
    "${CMAKE_SOURCE_DIR}/src/mms/server/main.cc"
)

# Create a shared library for common sources (compiled once, linked multiple times)
add_library(mms_common STATIC ${COMMON_SRC})
set_target_properties(mms_common PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
)
target_compile_features(mms_common PRIVATE cxx_std_23)
target_include_directories(mms_common PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}
  ${quicklib_SOURCE_DIR}/include
)
target_link_libraries(mms_common PUBLIC
  nlohmann_json::nlohmann_json
  # toml11::toml11
)

add_executable(client src/mms/client/main.cc)
set_target_properties(client PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
)

add_executable(server src/mms/server/main.cc)
set_target_properties(server PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED YES
)

# Make sure we have C++23
target_compile_features(server PRIVATE cxx_std_23)
target_compile_features(client PRIVATE cxx_std_23)

target_link_libraries(client PRIVATE
  mms_common
)

target_link_libraries(server PRIVATE
  mms_common
)

target_include_directories(server PRIVATE 
  ${CMAKE_SOURCE_DIR}/include
)


target_include_directories(client PRIVATE 
  ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# Testing Configuration
# ============================================================================

file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_SOURCE_DIR}/tests/*_test.cc"
    "${CMAKE_SOURCE_DIR}/tests/test.cc"
)

add_executable(unit_tests
    ${TEST_SOURCES}
)

set_target_properties(unit_tests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED YES
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests        # For test utilities and headers
    ${quicklib_SOURCE_DIR}/include
)

target_link_libraries(unit_tests PRIVATE
    mms_common
    GTest::gtest
    GTest::gtest_main
)

# Register tests with CTest
add_test(NAME unit_tests COMMAND unit_tests)

# ============================================================================
# Benchmarking Configuration
# ============================================================================

find_package(benchmark REQUIRED)

# Collect all benchmark source files
file(GLOB_RECURSE BENCHMARK_SOURCES
    "${CMAKE_SOURCE_DIR}/benchmarks/*_benchmark.cc"
)

if(BENCHMARK_SOURCES)
    add_executable(benchmarks
        ${BENCHMARK_SOURCES}
    )

    set_target_properties(benchmarks PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED YES
    )

    target_include_directories(benchmarks PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/benchmarks   # For benchmark utilities
        ${quicklib_SOURCE_DIR}/include
    )

    target_link_libraries(benchmarks PRIVATE
        mms_common
        benchmark::benchmark
        benchmark::benchmark_main
    )
endif()

# Optional: Use gtest_discover_tests for better test reporting
gtest_discover_tests(unit_tests)
