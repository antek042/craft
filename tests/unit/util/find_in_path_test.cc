#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "scoped_path.h"
#include "temp_dir.h"

import craft.util;

namespace {

namespace fs = std::filesystem;

#ifdef _WIN32
constexpr char kSeparator = ';';
constexpr const char* kProgram = "fake_tool.exe";
#else
constexpr char kSeparator = ':';
constexpr const char* kProgram = "fake_tool";
#endif

void MakeProgram(const fs::path& dir, bool executable) {
    fs::create_directories(dir);
    const auto file = dir / kProgram;
    std::ofstream(file) << "\n";
#ifndef _WIN32
    fs::permissions(file,
                    executable ? fs::perms::owner_all : fs::perms::owner_read);
#else
    (void)executable;
#endif
}

constexpr const char* kName = "fake_tool";

}  // namespace

TEST_CASE("FindInPath finds a program in PATH", "[util][find_in_path]") {
    const TempDir dir;
    MakeProgram(dir.path() / "bin", true);
    const ScopedPath path((dir.path() / "bin").string());

    const auto found = craft::util::FindInPath(kName);

    CHECK(found == dir.path() / "bin" / kProgram);
}

TEST_CASE("FindInPath returns nothing for a missing program",
          "[util][find_in_path]") {
    const TempDir dir;
    const ScopedPath path(dir.path().string());

    CHECK_FALSE(craft::util::FindInPath(kName).has_value());
}

TEST_CASE("FindInPath takes the first directory that has the program",
          "[util][find_in_path]") {
    const TempDir dir;
    MakeProgram(dir.path() / "first", true);
    MakeProgram(dir.path() / "second", true);
    const ScopedPath path((dir.path() / "first").string() + kSeparator +
                          (dir.path() / "second").string());

    const auto found = craft::util::FindInPath(kName);

    CHECK(found == dir.path() / "first" / kProgram);
}

TEST_CASE("FindInPath skips empty entries and missing directories",
          "[util][find_in_path]") {
    const TempDir dir;
    MakeProgram(dir.path() / "bin", true);
    const ScopedPath path(std::string(1, kSeparator) +
                          (dir.path() / "nope").string() + kSeparator +
                          (dir.path() / "bin").string());

    const auto found = craft::util::FindInPath(kName);

    CHECK(found == dir.path() / "bin" / kProgram);
}

#ifndef _WIN32
TEST_CASE("FindInPath ignores a file that is not executable",
          "[util][find_in_path]") {
    const TempDir dir;
    MakeProgram(dir.path() / "bin", false);
    const ScopedPath path((dir.path() / "bin").string());

    CHECK_FALSE(craft::util::FindInPath(kName).has_value());
}
#endif
