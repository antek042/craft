cmake_minimum_required(VERSION 3.28)

project(craft
    VERSION 0.1.0
    DESCRIPTION "A build tool and project manager for C++"
    LANGUAGES CXX)

if(CMAKE_GENERATOR MATCHES "Makefiles")
    message(FATAL_ERROR
        "Craft uses C++20 modules, which the Makefile generator doesn't "
        "support. Configure with Ninja instead: cmake -B build -G Ninja")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CRAFT_BUILD_TESTS "Build Craft's tests" ${PROJECT_IS_TOP_LEVEL})
option(CRAFT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(CRAFT_SANITIZE "Build with AddressSanitizer and UBSan" OFF)

# Warning flags for Craft's own targets (not for third-party code).
add_library(craft_warnings INTERFACE)
if(MSVC)
    target_compile_options(craft_warnings INTERFACE /W4 /permissive-)
    if(CRAFT_WARNINGS_AS_ERRORS)
        target_compile_options(craft_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(craft_warnings INTERFACE -Wall -Wextra -Wpedantic)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # CMake passes `-c` when it only builds a module interface (BMI),
        # which Clang reports as unused. Not a problem in our code.
        target_compile_options(craft_warnings
            INTERFACE -Wno-unused-command-line-argument)
    endif()
    if(CRAFT_WARNINGS_AS_ERRORS)
        target_compile_options(craft_warnings INTERFACE -Werror)
    endif()
endif()

if(CRAFT_SANITIZE)
    if(MSVC)
        message(FATAL_ERROR "CRAFT_SANITIZE is only supported with GCC and Clang")
    endif()
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(src)

if(CRAFT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
