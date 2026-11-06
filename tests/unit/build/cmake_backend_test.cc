#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "temp_dir.h"

import craft.build;
import craft.manifest;

namespace {

namespace fs = std::filesystem;

std::string ReadFile(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
}

craft::manifest::Manifest MakeManifest() {
    craft::manifest::Manifest manifest;
    manifest.package.name = "hello";
    manifest.package.version = "0.1.0";
    return manifest;
}

craft::build::Targets MakeTargets(const fs::path& root) {
    craft::build::Targets targets;
    targets.executable = craft::build::Target{.name = "hello",
                                              .module_name = "",
                                              .entry = root / "src/main.cc",
                                              .sources = {root / "src/main.cc"},
                                              .modules = {}};
    return targets;
}

}  // namespace

TEST_CASE("WriteIfChanged creates the file and its directories",
          "[build][write_if_changed]") {
    const TempDir dir;
    const auto file = dir.path() / "a" / "b" / "CMakeLists.txt";

    const auto result = craft::build::WriteIfChanged(file, "text\n");

    REQUIRE(result.has_value());
    CHECK(*result);
    CHECK(ReadFile(file) == "text\n");
}

TEST_CASE("WriteIfChanged leaves identical content alone",
          "[build][write_if_changed]") {
    const TempDir dir;
    const auto file = dir.path() / "CMakeLists.txt";
    REQUIRE(craft::build::WriteIfChanged(file, "text\n").has_value());

    const auto again = craft::build::WriteIfChanged(file, "text\n");

    REQUIRE(again.has_value());
    CHECK_FALSE(*again);
}

TEST_CASE("WriteIfChanged replaces different content",
          "[build][write_if_changed]") {
    const TempDir dir;
    const auto file = dir.path() / "CMakeLists.txt";
    REQUIRE(craft::build::WriteIfChanged(file, "old\n").has_value());

    const auto result = craft::build::WriteIfChanged(file, "new\n");

    REQUIRE(result.has_value());
    CHECK(*result);
    CHECK(ReadFile(file) == "new\n");
}

TEST_CASE("WriteIfChanged reports a path it can't write",
          "[build][write_if_changed]") {
    const TempDir dir;
    // A directory where the file should be.
    const auto file = dir.path() / "CMakeLists.txt";
    fs::create_directories(file);

    const auto result = craft::build::WriteIfChanged(file, "text\n");

    CHECK_FALSE(result.has_value());
}

TEST_CASE("Generate writes target/cmake/CMakeLists.txt",
          "[build][cmake_backend]") {
    const TempDir dir;
    craft::build::CMakeBackend backend;
    const craft::build::BuildOptions options{.root = dir.path()};

    const auto result =
        backend.Generate(MakeManifest(), MakeTargets(dir.path()), options);

    REQUIRE(result.has_value());
    const auto text = ReadFile(dir.path() / "target/cmake/CMakeLists.txt");
    CHECK(text == craft::build::GenerateCMakeText(
                      MakeManifest(), MakeTargets(dir.path()),
                      craft::build::Profile::kDebug));
}

TEST_CASE("Generate twice gives the same file", "[build][cmake_backend]") {
    const TempDir dir;
    craft::build::CMakeBackend backend;
    const craft::build::BuildOptions options{.root = dir.path()};
    const auto file = dir.path() / "target/cmake/CMakeLists.txt";

    REQUIRE(backend.Generate(MakeManifest(), MakeTargets(dir.path()), options)
                .has_value());
    const auto first = ReadFile(file);
    REQUIRE(backend.Generate(MakeManifest(), MakeTargets(dir.path()), options)
                .has_value());

    CHECK(ReadFile(file) == first);
}
