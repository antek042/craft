#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "temp_dir.h"

import craft.cli;
import craft.core;
import craft.manifest;

namespace {

namespace fs = std::filesystem;

struct InitResult {
    int exit_code;
    std::string out;
    std::string err;
};

InitResult RunInit(const fs::path& dir) {
    std::ostringstream out;
    std::ostringstream err;
    const int exit_code = craft::cli::InitProject(dir, out, err);
    return {.exit_code = exit_code, .out = out.str(), .err = err.str()};
}

// A project directory with the given name inside a fresh temporary directory.
fs::path MakeProjectDir(const TempDir& root, const std::string& name) {
    const fs::path dir = root.path() / name;
    fs::create_directories(dir);
    return dir;
}

void WriteFile(const fs::path& file, const std::string& text) {
    std::ofstream(file, std::ios::binary) << text;
}

}  // namespace

TEST_CASE("init creates a project named after the directory", "[cli][init]") {
    const TempDir root;
    const fs::path dir = MakeProjectDir(root, "hello-app");

    const InitResult result = RunInit(dir);

    CHECK(result.exit_code == 0);
    CHECK(result.err.empty());
    CHECK(result.out ==
          "Created binary package hello-app\n"
          "  created  craft.toml\n"
          "  created  src/main.cpp\n"
          "  created  .gitignore\n");
    const auto manifest = craft::manifest::LoadManifest(dir / "craft.toml");
    REQUIRE(manifest.has_value());
    CHECK(manifest->package.name == "hello-app");
}

TEST_CASE("init converts the directory name to lowercase", "[cli][init]") {
    const TempDir root;
    const fs::path dir = MakeProjectDir(root, "Hello_World");

    const InitResult result = RunInit(dir);

    CHECK(result.exit_code == 0);
    const auto manifest = craft::manifest::LoadManifest(dir / "craft.toml");
    REQUIRE(manifest.has_value());
    CHECK(manifest->package.name == "hello_world");
}

TEST_CASE("init lists the files it left alone", "[cli][init]") {
    const TempDir root;
    const fs::path dir = MakeProjectDir(root, "app");
    WriteFile(dir / ".gitignore", "build/\n");

    const InitResult result = RunInit(dir);

    CHECK(result.exit_code == 0);
    CHECK(result.out ==
          "Created binary package app\n"
          "  created  craft.toml\n"
          "  created  src/main.cpp\n"
          "  skipped  .gitignore (already exists)\n");
}

TEST_CASE("init refuses a directory that already has craft.toml",
          "[cli][init]") {
    const TempDir root;
    const fs::path dir = MakeProjectDir(root, "app");
    WriteFile(dir / "craft.toml", "# mine\n");

    const InitResult result = RunInit(dir);

    CHECK(result.exit_code == 1);
    CHECK(result.out.empty());
    CHECK(result.err.contains("`craft.toml` already exists"));
    CHECK(result.err.contains("hint:"));
    CHECK_FALSE(fs::exists(dir / "src"));
}

TEST_CASE("init fails for a directory name that is not a package name",
          "[cli][init]") {
    const TempDir root;
    const fs::path dir = MakeProjectDir(root, "My App");

    const InitResult result = RunInit(dir);

    CHECK(result.exit_code == 1);
    CHECK(result.out.empty());
    CHECK(result.err.contains(
        "error: cannot derive a package name from directory `My App`"));
    CHECK(result.err.contains("hint: rename the directory"));
    CHECK(fs::is_empty(dir));
}

TEST_CASE("craft init takes no arguments", "[cli][init]") {
    std::ostringstream out;
    std::ostringstream err;
    const std::vector<std::string_view> args = {"init", "extra"};

    const int exit_code = craft::cli::Run(args, out, err);

    CHECK(exit_code == 1);
    CHECK(out.str().empty());
    CHECK(err.str().contains("`craft init` takes no arguments, got `extra`"));
}

TEST_CASE("the help text lists init", "[cli][init]") {
    std::ostringstream out;
    std::ostringstream err;
    const std::vector<std::string_view> args = {"--help"};

    CHECK(craft::cli::Run(args, out, err) == 0);
    CHECK(out.str().contains("init"));
}
