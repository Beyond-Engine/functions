cmake_minimum_required(VERSION 3.12)

project(unique_function VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BEYOND_UNIQUE_FUNCTION_NAMESPACE "beyond" CACHE STRING "namespace for wrapping the unique_funciton")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include("CMakeDependentOption")

option(BEYOND_UNIQUE_FUNCTION_BUILD_TESTS "Builds the tests" OFF)
CMAKE_DEPENDENT_OPTION(BEYOND_UNIQUE_FUNCTION_BUILD_TESTS_COVERAGE
    "Build the project with code coverage support for tests" OFF
                       "BEYOND_UNIQUE_FUNCTION_BUILD_TESTS" OFF)

option(BEYOND_UNIQUE_FUNCTION_WARNING_AS_ERROR "Treats compiler warnings as errors" ON)
option(BEYOND_UNIQUE_FUNCTION_NO_EXCEPTION "Do not throw exception in the unique_function implementation" OFF)

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
target_compile_definitions(unique_function INTERFACE
    BEYOND_UNIQUE_FUNCTION_NAMESPACE=${BEYOND_UNIQUE_FUNCTION_NAMESPACE})
if (${BEYOND_UNIQUE_FUNCTION_NO_EXCEPTION})
    target_compile_definitions(unique_function INTERFACE
        BEYOND_UNIQUE_FUNCTION_NO_EXCEPTION)
endif()

if(BEYOND_UNIQUE_FUNCTION_BUILD_TESTS)
    add_subdirectory(test)
endif()
