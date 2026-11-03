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
    unit/cli/cli_test.cc
    unit/cli/create_project_test.cc
    unit/manifest/default_manifest_test.cc
    unit/manifest/loader_test.cc
    unit/manifest/name_test.cc
    unit/manifest/parser_test.cc)

# Exceptions to craft_warnings needed by test code. Linked after
# craft_warnings so these flags come later on the command line and win.
add_library(craft_test_warnings INTERFACE)

# Catch2's TEST_CASE macro uses __COUNTER__, which Clang 22+ reports under
# -Wpedantic. Older compilers don't know this warning, so check first.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wc2y-extensions CRAFT_HAS_WC2Y_EXTENSIONS)
if(CRAFT_HAS_WC2Y_EXTENSIONS)
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
    PASS_REGULAR_EXPRESSION "^craft [0-9]+\\.[0-9]+\\.[0-9]+\r?\n$")

add_test(NAME integration.unknown_command COMMAND craft frobnicate)
set_tests_properties(integration.unknown_command PROPERTIES WILL_FAIL TRUE)

# `craft read-manifest` run inside the sample projects in tests/fixtures/.
set(CRAFT_FIXTURES ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)

add_test(NAME integration.read_manifest.valid
    COMMAND craft read-manifest
    WORKING_DIRECTORY ${CRAFT_FIXTURES}/valid)
set_tests_properties(integration.read_manifest.valid PROPERTIES
    PASS_REGULAR_EXPRESSION
        "^name = hello\r?\nversion = 0\\.1\\.0\r?\ncpp = 23\r?\n$")

# A failing case needs two tests: with PASS_REGULAR_EXPRESSION CTest ignores
# the exit code, so the exit code and the message are checked separately.
function(add_read_manifest_failure name fixture message_regex)
    add_test(NAME integration.read_manifest.${name}.exit_code
        COMMAND craft read-manifest
        WORKING_DIRECTORY ${CRAFT_FIXTURES}/${fixture})
    set_tests_properties(integration.read_manifest.${name}.exit_code
        PROPERTIES WILL_FAIL TRUE)

    add_test(NAME integration.read_manifest.${name}.message
        COMMAND craft read-manifest
        WORKING_DIRECTORY ${CRAFT_FIXTURES}/${fixture})
    set_tests_properties(integration.read_manifest.${name}.message
        PROPERTIES PASS_REGULAR_EXPRESSION "${message_regex}")
endfunction()

add_read_manifest_failure(missing_package missing-package
    "error: missing \\[package\\] section")
add_read_manifest_failure(typo_in_key typo-key
    "error: missing key `name` in \\[package\\]")
