#include <chrono>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>

#include <catch2/catch_test_macros.hpp>

import craft.core;
import craft.manifest;

namespace {

namespace fs = std::filesystem;

// A fresh temporary directory, removed when the test ends.
class TempDir {
public:
    TempDir() {
        static int counter = 0;
        // Not file_time_type::clock: its tick type differs between standard
        // libraries (libc++ uses __int128, which std::to_string can't take).
        const auto nanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        path_ = fs::temp_directory_path() /
                ("craft_loader_test_" + std::to_string(counter++) + "_" +
                 std::to_string(nanoseconds));
        fs::create_directories(path_);
        // Resolve symlinks (e.g. /tmp on macOS) so paths compare equal.
        path_ = fs::canonical(path_);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

void Touch(const fs::path& file) { std::ofstream(file) << "[package]\n"; }

void WriteFile(const fs::path& file, const std::string& text) {
    std::ofstream(file, std::ios::binary) << text;
}

}  // namespace

TEST_CASE("manifest is found in the starting directory", "[manifest]") {
    const TempDir dir;
    Touch(dir.path() / "craft.toml");

    const auto result = craft::manifest::FindManifest(dir.path());

    REQUIRE(result.has_value());
    CHECK(*result == dir.path() / "craft.toml");
}

TEST_CASE("manifest is found in a parent directory", "[manifest]") {
    const TempDir dir;
    Touch(dir.path() / "craft.toml");
    const fs::path nested = dir.path() / "src" / "deep";
    fs::create_directories(nested);

    const auto result = craft::manifest::FindManifest(nested);

    REQUIRE(result.has_value());
    CHECK(*result == dir.path() / "craft.toml");
}

TEST_CASE("manifest is found when starting from a file", "[manifest]") {
    const TempDir dir;
    Touch(dir.path() / "craft.toml");
    fs::create_directories(dir.path() / "src");
    Touch(dir.path() / "src" / "main.cc");

    const auto result =
        craft::manifest::FindManifest(dir.path() / "src" / "main.cc");

    REQUIRE(result.has_value());
    CHECK(*result == dir.path() / "craft.toml");
}

TEST_CASE("the nearest manifest wins", "[manifest]") {
    const TempDir dir;
    Touch(dir.path() / "craft.toml");
    fs::create_directories(dir.path() / "inner");
    Touch(dir.path() / "inner" / "craft.toml");

    const auto result = craft::manifest::FindManifest(dir.path() / "inner");

    REQUIRE(result.has_value());
    CHECK(*result == dir.path() / "inner" / "craft.toml");
}

TEST_CASE("missing manifest gives an error with a hint", "[manifest]") {
    const TempDir dir;

    const auto result = craft::manifest::FindManifest(dir.path());

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "could not find `craft.toml` in `" +
                                          dir.path().string() +
                                          "` or any parent directory");
    CHECK(result.error().hint() ==
          "run `craft init` to create a new project here");
}

TEST_CASE("manifest is loaded and parsed", "[manifest]") {
    const TempDir dir;
    const fs::path file = dir.path() / "craft.toml";
    WriteFile(file,
              "[package]\n"
              "name = \"hello\"\n"
              "version = \"0.1.0\"\n");

    const auto result = craft::manifest::LoadManifest(file);

    REQUIRE(result.has_value());
    CHECK(result->package.name == "hello");
    CHECK(result->package.version == "0.1.0");
}

TEST_CASE("loading a missing file gives an error", "[manifest]") {
    const TempDir dir;
    const fs::path file = dir.path() / "craft.toml";

    const auto result = craft::manifest::LoadManifest(file);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().starts_with("could not read `" +
                                               file.string() + "`"));
}

TEST_CASE("loading an empty file reports the missing section", "[manifest]") {
    const TempDir dir;
    const fs::path file = dir.path() / "craft.toml";
    WriteFile(file, "");

    const auto result = craft::manifest::LoadManifest(file);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "missing [package] section");
}

TEST_CASE("parse errors point at the real file", "[manifest]") {
    const TempDir dir;
    const fs::path file = dir.path() / "craft.toml";
    WriteFile(file, "[package\n");

    const auto result = craft::manifest::LoadManifest(file);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().Format().contains("--> " + file.string() + ":1:"));
}

#ifndef _WIN32
TEST_CASE("loading an unreadable file gives an error with a hint",
          "[manifest]") {
    const TempDir dir;
    const fs::path file = dir.path() / "craft.toml";
    WriteFile(file, "[package]\n");
    fs::permissions(file, fs::perms::none);
    if (std::ifstream(file).is_open()) {
        SKIP("permissions are not enforced (running as root?)");
    }

    const auto result = craft::manifest::LoadManifest(file);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "could not open `" + file.string() + "`");
    CHECK(result.error().hint().contains("permission"));
}
#endif
