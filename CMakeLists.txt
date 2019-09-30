cmake_minimum_required(VERSION 3.8)

project(unique_function VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include("CMakeDependentOption")

option(BEYOND_UNIQUE_FUNCTION_BUILD_TESTS "Builds the tests" OFF)
CMAKE_DEPENDENT_OPTION(BEYOND_UNIQUE_FUNCTION_BUILD_TESTS_COVERAGE
    "Build the project with code coverage support for tests" OFF
                       "BEYOND_UNIQUE_FUNCTION_BUILD_TESTS" OFF)

option(BEYOND_UNIQUE_FUNCTION_WARNING_AS_ERROR "Treats compiler warnings as errors" ON)

include("compiler")

# Conan package manager
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

add_library(unique_function INTERFACE)
target_sources(unique_function INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/unique_function.hpp")
target_include_directories(unique_function INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(BEYOND_UNIQUE_FUNCTION_BUILD_TESTS)
    add_subdirectory(test)
endif()
