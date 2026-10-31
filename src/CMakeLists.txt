# The version partition is generated so that the version lives in one place:
# project(VERSION ...) in the top-level CMakeLists.txt.
configure_file(
    craft/core/version.cppm.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/craft/core/version.cppm
    @ONLY)

# All of Craft except main(). Tests link against this library.
add_library(craft_lib STATIC)
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
    PRIVATE
        craft/core/error.cc
        craft/cli/cli.cc)
target_link_libraries(craft_lib PRIVATE craft_warnings)

add_executable(craft main.cc)
target_link_libraries(craft PRIVATE craft_lib craft_warnings)
