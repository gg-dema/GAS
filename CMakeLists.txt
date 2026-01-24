cmake_minimum_required(VERSION 3.30)
project(GAS LANGUAGES C CXX)  # Note: Added C language

enable_language(C)  # Explicitly enable C
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)



if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  MESSAGE("Clang")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  MESSAGE("GNU")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  MESSAGE("Intel")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  MESSAGE("MSVC")
endif()

# Dependencies
add_subdirectory(external/glfw)
add_subdirectory(external/glm)

# GLAD
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# openGL
find_package(OpenGL REQUIRED)

# Main project
add_subdirectory(src)