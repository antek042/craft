include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW TRUE
    SYSTEM)
FetchContent_MakeAvailable(Catch2)

# Unit tests: one executable with all of them.
add_executable(craft_unit_tests
    unit/core/error_test.cc
    unit/cli/cli_test.cc)
# Exceptions to craft_warnings needed by test code. Linked after
# craft_warnings so these flags come later on the command line and win.
add_library(craft_test_warnings INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Catch2's TEST_CASE macro uses __COUNTER__, which Clang 22+ reports
    # under -Wpedantic.
    target_compile_options(craft_test_warnings INTERFACE -Wno-c2y-extensions)
endif()

target_link_libraries(craft_unit_tests
    PRIVATE
        craft_lib
        craft_warnings
        craft_test_warnings
        Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(craft_unit_tests)

# Integration tests: run the real `craft` binary.
add_test(NAME integration.version COMMAND craft --version)
set_tests_properties(integration.version PROPERTIES
    PASS_REGULAR_EXPRESSION "^craft [0-9]+\\.[0-9]+\\.[0-9]+\n$")

add_test(NAME integration.unknown_command COMMAND craft frobnicate)
set_tests_properties(integration.unknown_command PROPERTIES WILL_FAIL TRUE)
