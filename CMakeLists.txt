cmake_minimum_required(VERSION 3.20)
project(GAS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project structure
include_directories(${CMAKE_SOURCE_DIR}/include)

# Main executable
add_executable(gas
  src/main.cpp
)

# GoogleTest
enable_testing()
add_subdirectory(tests)
