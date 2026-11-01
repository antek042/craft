# The version partition is generated so that the version lives in one place:
# project(VERSION ...) in the top-level CMakeLists.txt.
configure_file(
    craft/core/version.cppm.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/craft/core/version.cppm
    @ONLY)

include(FetchContent)

FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
    GIT_SHALLOW TRUE
    SYSTEM)
FetchContent_MakeAvailable(tomlplusplus)

# All of Craft except main(). Tests link against this library.
add_library(craft_lib STATIC)

target_link_libraries(craft_lib PRIVATE craft_warnings tomlplusplus::tomlplusplus)
target_compile_definitions(craft_lib PRIVATE TOML_EXCEPTIONS=0)

target_sources(craft_lib
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/generated
        FILES
            craft/core/core.cppm
            craft/core/error.cppm
            ${CMAKE_CURRENT_BINARY_DIR}/generated/craft/core/version.cppm
            craft/cli/cli.cppm
            craft/manifest/manifest.cppm
            craft/manifest/parser.cppm
            craft/manifest/types.cppm
    PRIVATE
        craft/core/error.cc
        craft/cli/cli.cc
        craft/manifest/parser.cc)
target_link_libraries(craft_lib PRIVATE craft_warnings)

add_executable(craft main.cc)
target_link_libraries(craft PRIVATE craft_lib craft_warnings)
