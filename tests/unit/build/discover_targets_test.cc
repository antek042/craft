#include <exception>
#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "temp_dir.h"

import craft.build;
import craft.core;
import craft.manifest;

namespace {

namespace fs = std::filesystem;

void Touch(const fs::path& root, const std::string& relative) {
    const fs::path file = root / relative;
    fs::create_directories(file.parent_path());
    std::ofstream(file) << "\n";
}

std::string ManifestNamed(const std::string& name) { return name; }

// REQUIRE does not tell clang-tidy that the optional is set, so the check is
// written as a plain `if`.
const craft::build::Target& Unwrap(
    const std::optional<craft::build::Target>& target) {
    if (!target.has_value()) {
        FAIL("target is missing");
        std::terminate();
    }
    return *target;
}

const craft::build::Target& Library(const craft::build::Targets& targets) {
    return Unwrap(targets.library);
}

const craft::build::Target& Program(const craft::build::Targets& targets) {
    return Unwrap(targets.executable);
}

}  // namespace

TEST_CASE("only main.cc gives a program", "[build][discover_targets]") {
    const TempDir dir;
    Touch(dir.path(), "src/main.cc");

    const auto result =
        craft::build::DiscoverTargets(dir.path(), ManifestNamed("hello"));

    REQUIRE(result.has_value());
    CHECK_FALSE(result->library.has_value());
    REQUIRE(result->executable.has_value());
    CHECK(Program(*result).name == "hello");
    CHECK(Program(*result).entry == dir.path() / "src/main.cc");
}

TEST_CASE("only lib.cppm gives a library", "[build][discover_targets]") {
    const TempDir dir;
    Touch(dir.path(), "src/lib.cppm");

    const auto result =
        craft::build::DiscoverTargets(dir.path(), ManifestNamed("hello"));

    REQUIRE(result.has_value());
    CHECK_FALSE(result->executable.has_value());
    REQUIRE(result->library.has_value());
    CHECK(Library(*result).name == "hello");
    CHECK(Library(*result).module_name == "hello");
    CHECK(Library(*result).entry == dir.path() / "src/lib.cppm");
}

TEST_CASE("main and lib give both; other files go to the library",
          "[build][discover_targets]") {
    const TempDir dir;
    Touch(dir.path(), "src/main.cc");
    Touch(dir.path(), "src/lib.cppm");
    Touch(dir.path(), "src/util.cc");

    const auto result =
        craft::build::DiscoverTargets(dir.path(), ManifestNamed("hello"));

    REQUIRE(result.has_value());
    REQUIRE(result->library.has_value());
    REQUIRE(result->executable.has_value());
    CHECK(Library(*result).sources ==
          std::vector<fs::path>{dir.path() / "src/util.cc"});
    CHECK(Library(*result).modules ==
          std::vector<fs::path>{dir.path() / "src/lib.cppm"});
    CHECK(Program(*result).sources ==
          std::vector<fs::path>{dir.path() / "src/main.cc"});
    CHECK(Program(*result).modules.empty());
}

TEST_CASE("without a library other files go to the program",
          "[build][discover_targets]") {
    const TempDir dir;
    Touch(dir.path(), "src/main.cc");
    Touch(dir.path(), "src/util.cc");
    Touch(dir.path(), "src/extra.cppm");

    const auto result =
        craft::build::DiscoverTargets(dir.path(), ManifestNamed("hello"));

    REQUIRE(result.has_value());
    REQUIRE(result->executable.has_value());
    CHECK(Program(*result).sources.size() == 2);
    CHECK(Program(*result).modules ==
          std::vector<fs::path>{dir.path() / "src/extra.cppm"});
}

TEST_CASE("neither main nor lib is an error", "[build][discover_targets]") {
    const TempDir dir;
    Touch(dir.path(), "src/util.cc");

    const auto result =
        craft::build::DiscoverTargets(dir.path(), ManifestNamed("hello"));

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "no targets found");
    CHECK(result.error().hint() == "create src/main.cc or src/lib.cppm");
}

TEST_CASE("a missing src directory is an error", "[build][discover_targets]") {
    const TempDir dir;

    const auto result =
        craft::build::DiscoverTargets(dir.path(), ManifestNamed("hello"));

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "no targets found");
}

TEST_CASE("nested files are found, hidden ones are not",
          "[build][discover_targets]") {
    const TempDir dir;
    Touch(dir.path(), "src/lib.cppm");
    Touch(dir.path(), "src/a/b/deep.cc");
    Touch(dir.path(), "src/a/part.cppm");
    Touch(dir.path(), "src/.hidden/skip.cc");

    const auto result =
        craft::build::DiscoverTargets(dir.path(), ManifestNamed("hello"));

    REQUIRE(result.has_value());
    REQUIRE(result->library.has_value());
    CHECK(Library(*result).sources ==
          std::vector<fs::path>{dir.path() / "src/a/b/deep.cc"});
    CHECK(Library(*result).modules.size() == 2);
}

TEST_CASE("a dash in the package name becomes _ in the module name",
          "[build][discover_targets]") {
    const TempDir dir;
    Touch(dir.path(), "src/lib.cppm");

    const auto result =
        craft::build::DiscoverTargets(dir.path(), ManifestNamed("my-lib"));

    REQUIRE(result.has_value());
    REQUIRE(result->library.has_value());
    CHECK(Library(*result).name == "my-lib");
    CHECK(Library(*result).module_name == "my_lib");
}

TEST_CASE("two main files are an error", "[build][discover_targets]") {
    const TempDir dir;
    Touch(dir.path(), "src/main.cc");
    Touch(dir.path(), "src/main.cpp");

    const auto result =
        craft::build::DiscoverTargets(dir.path(), ManifestNamed("hello"));

    REQUIRE_FALSE(result.has_value());
}
