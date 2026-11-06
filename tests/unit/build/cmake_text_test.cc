#include <optional>
#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

import craft.build;
import craft.manifest;

namespace {

using craft::build::Profile;
using craft::build::Target;
using craft::build::Targets;
using craft::manifest::CppStandard;

craft::manifest::Manifest MakeManifest(
    const std::string& name, CppStandard standard = CppStandard::k23) {
    craft::manifest::Manifest manifest;
    manifest.package.name = name;
    manifest.package.version = "0.1.0";
    manifest.package.cpp_standard = standard;
    return manifest;
}

Target Program(const std::string& name) {
    Target target;
    target.name = name;
    target.entry = "/p/src/main.cc";
    target.sources = {"/p/src/main.cc"};
    return target;
}

Target Library(const std::string& name) {
    Target target;
    target.name = name;
    target.module_name = name;
    target.entry = "/p/src/lib.cppm";
    target.modules = {"/p/src/lib.cppm"};
    return target;
}

bool Has(const std::string& text, const std::string& part) {
    return text.contains(part);
}

}  // namespace

TEST_CASE("a program only", "[build][cmake_text]") {
    Targets targets;
    targets.executable = Program("hello");

    const auto text = craft::build::GenerateCMakeText(MakeManifest("hello"),
                                                      targets, Profile::kDebug);

    CHECK(Has(text, "cmake_minimum_required(VERSION 3.28)"));
    CHECK(Has(text, "project(hello VERSION 0.1.0 LANGUAGES CXX)"));
    CHECK(Has(text, "add_executable(hello)"));
    CHECK(Has(text, "\"/p/src/main.cc\""));
    CHECK_FALSE(Has(text, "add_library"));
    CHECK_FALSE(Has(text, "target_link_libraries"));
}

TEST_CASE("a library only", "[build][cmake_text]") {
    Targets targets;
    targets.library = Library("hello");

    const auto text = craft::build::GenerateCMakeText(MakeManifest("hello"),
                                                      targets, Profile::kDebug);

    CHECK(Has(text, "add_library(hello_lib)"));
    CHECK(Has(text, "OUTPUT_NAME \"hello\""));
    CHECK(Has(text, "FILE_SET CXX_MODULES"));
    CHECK(Has(text, "BASE_DIRS \"/p/src\""));
    CHECK(Has(text, "\"/p/src/lib.cppm\""));
    CHECK_FALSE(Has(text, "add_executable"));
}

TEST_CASE("a program is linked with the library", "[build][cmake_text]") {
    Targets targets;
    targets.library = Library("hello");
    targets.executable = Program("hello");

    const auto text = craft::build::GenerateCMakeText(MakeManifest("hello"),
                                                      targets, Profile::kDebug);

    CHECK(Has(text, "add_library(hello_lib)"));
    CHECK(Has(text, "add_executable(hello)"));
    CHECK(Has(text, "target_link_libraries(hello PRIVATE hello_lib)"));
}

TEST_CASE("normal sources of a library are not in the module file set",
          "[build][cmake_text]") {
    Targets targets;
    targets.library = Library("hello");
    targets.library->sources = {"/p/src/util.cc"};

    const auto text = craft::build::GenerateCMakeText(MakeManifest("hello"),
                                                      targets, Profile::kDebug);

    const auto sources = text.find("\"/p/src/util.cc\"");
    const auto file_set = text.find("FILE_SET CXX_MODULES");
    REQUIRE(sources != std::string::npos);
    REQUIRE(file_set != std::string::npos);
    CHECK(sources < file_set);
}

TEST_CASE("a library without files is an interface library",
          "[build][cmake_text]") {
    Targets targets;
    targets.library = Target{.name = "hello",
                             .module_name = "hello",
                             .entry = "/p/src/lib.hpp",
                             .sources = {},
                             .modules = {}};

    const auto text = craft::build::GenerateCMakeText(MakeManifest("hello"),
                                                      targets, Profile::kDebug);

    CHECK(Has(text, "add_library(hello_lib INTERFACE)"));
    CHECK_FALSE(Has(text, "target_sources"));
}

TEST_CASE("a dashed package name is kept", "[build][cmake_text]") {
    Targets targets;
    targets.library = Library("my-lib");
    targets.executable = Program("my-lib");

    const auto text = craft::build::GenerateCMakeText(MakeManifest("my-lib"),
                                                      targets, Profile::kDebug);

    CHECK(Has(text, "project(my-lib VERSION 0.1.0 LANGUAGES CXX)"));
    CHECK(Has(text, "add_executable(my-lib)"));
    CHECK(Has(text, "add_library(my-lib_lib)"));
    CHECK(Has(text, "OUTPUT_NAME \"my-lib\""));
}

TEST_CASE("every C++ standard", "[build][cmake_text]") {
    Targets targets;
    targets.executable = Program("hello");

    const auto [standard, number] = GENERATE(
        std::pair{CppStandard::k17, "17"}, std::pair{CppStandard::k20, "20"},
        std::pair{CppStandard::k23, "23"}, std::pair{CppStandard::k26, "26"});

    const auto text = craft::build::GenerateCMakeText(
        MakeManifest("hello", standard), targets, Profile::kDebug);

    CHECK(Has(text, std::string("set(CMAKE_CXX_STANDARD ") + number + ")"));
    CHECK(Has(text, "set(CMAKE_CXX_EXTENSIONS OFF)"));
}

TEST_CASE("the profile sets the build type", "[build][cmake_text]") {
    Targets targets;
    targets.executable = Program("hello");

    const auto debug = craft::build::GenerateCMakeText(
        MakeManifest("hello"), targets, Profile::kDebug);
    const auto release = craft::build::GenerateCMakeText(
        MakeManifest("hello"), targets, Profile::kRelease);

    CHECK(Has(debug, "set(CMAKE_BUILD_TYPE \"Debug\""));
    CHECK(Has(release, "set(CMAKE_BUILD_TYPE \"Release\""));
}

TEST_CASE("the same input gives the same text", "[build][cmake_text]") {
    Targets targets;
    targets.library = Library("hello");
    targets.executable = Program("hello");

    CHECK(craft::build::GenerateCMakeText(MakeManifest("hello"), targets,
                                          Profile::kDebug) ==
          craft::build::GenerateCMakeText(MakeManifest("hello"), targets,
                                          Profile::kDebug));
}
