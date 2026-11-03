#include <chrono>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

import craft.cli;
import craft.core;
import craft.manifest;

namespace {

namespace fs = std::filesystem;

// A fresh temporary directory, removed when the test ends.
class TempDir {
public:
    TempDir() {
        static int counter = 0;
        const auto nanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        path_ = fs::temp_directory_path() /
                ("craft_create_project_test_" + std::to_string(counter++) +
                 "_" + std::to_string(nanoseconds));
        fs::create_directories(path_);
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

std::string ReadFile(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
}

void WriteFile(const fs::path& file, const std::string& text) {
    std::ofstream(file, std::ios::binary) << text;
}

}  // namespace

TEST_CASE("a new project gets craft.toml, main.cpp and .gitignore",
          "[cli][create_project]") {
    const TempDir dir;

    const auto result = craft::cli::CreateProject(dir.path(), "hello");

    REQUIRE(result.has_value());
    CHECK(result->created ==
          std::vector<std::string>{"craft.toml", "src/main.cpp", ".gitignore"});
    CHECK(result->skipped.empty());
    CHECK(ReadFile(dir.path() / ".gitignore") == "/target\n");
    CHECK(ReadFile(dir.path() / "src" / "main.cpp") ==
          "#include <iostream>\n"
          "\n"
          "int main() {\n"
          "    std::cout << \"Hello, world!\\n\";\n"
          "}\n");
}

TEST_CASE("the generated manifest can be loaded", "[cli][create_project]") {
    const TempDir dir;

    REQUIRE(craft::cli::CreateProject(dir.path(), "my-app").has_value());
    const auto manifest =
        craft::manifest::LoadManifest(dir.path() / "craft.toml");

    REQUIRE(manifest.has_value());
    CHECK(manifest->package.name == "my-app");
    CHECK(manifest->package.version == "0.1.0");
}

TEST_CASE("existing files other than craft.toml are left untouched",
          "[cli][create_project]") {
    const TempDir dir;
    fs::create_directories(dir.path() / "src");
    WriteFile(dir.path() / "src" / "main.cpp", "// mine\n");
    WriteFile(dir.path() / ".gitignore", "build/\n");

    const auto result = craft::cli::CreateProject(dir.path(), "hello");

    REQUIRE(result.has_value());
    CHECK(result->created == std::vector<std::string>{"craft.toml"});
    CHECK(result->skipped ==
          std::vector<std::string>{"src/main.cpp", ".gitignore"});
    CHECK(ReadFile(dir.path() / "src" / "main.cpp") == "// mine\n");
    CHECK(ReadFile(dir.path() / ".gitignore") == "build/\n");
}

TEST_CASE("an existing craft.toml is an error and nothing is changed",
          "[cli][create_project]") {
    const TempDir dir;
    WriteFile(dir.path() / "craft.toml", "# keep me\n");

    const auto result = craft::cli::CreateProject(dir.path(), "hello");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("`craft.toml` already exists"));
    CHECK_FALSE(result.error().hint().empty());
    CHECK(ReadFile(dir.path() / "craft.toml") == "# keep me\n");
    CHECK_FALSE(fs::exists(dir.path() / "src"));
    CHECK_FALSE(fs::exists(dir.path() / ".gitignore"));
}

TEST_CASE("an invalid name is an error and nothing is created",
          "[cli][create_project]") {
    const TempDir dir;

    const auto result = craft::cli::CreateProject(dir.path(), "My App");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("invalid character"));
    CHECK(fs::is_empty(dir.path()));
}

TEST_CASE("a directory that does not exist is an error",
          "[cli][create_project]") {
    const TempDir dir;
    const fs::path missing = dir.path() / "nope";

    const auto result = craft::cli::CreateProject(missing, "hello");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() ==
          "directory `" + missing.string() + "` does not exist");
    CHECK_FALSE(fs::exists(missing));
}

TEST_CASE("a file instead of a directory is an error",
          "[cli][create_project]") {
    const TempDir dir;
    const fs::path file = dir.path() / "file.txt";
    WriteFile(file, "x");

    const auto result = craft::cli::CreateProject(file, "hello");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() ==
          "`" + file.string() + "` is not a directory");
}

#ifndef _WIN32
TEST_CASE("a write error removes what was created so far",
          "[cli][create_project]") {
    const TempDir dir;
    const fs::path src = dir.path() / "src";
    fs::create_directories(src);
    WriteFile(dir.path() / ".gitignore", "build/\n");
    // craft.toml can be written, but nothing can be created in src/.
    fs::permissions(src, fs::perms::owner_read | fs::perms::owner_exec);
    if (std::ofstream(src / "probe").is_open()) {
        fs::permissions(src, fs::perms::owner_all);
        SKIP("permissions are not enforced (running as root?)");
    }

    const auto result = craft::cli::CreateProject(dir.path(), "hello");
    fs::permissions(src, fs::perms::owner_all);  // so that cleanup can work

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("could not create"));
    CHECK_FALSE(fs::exists(dir.path() / "craft.toml"));
    CHECK(fs::exists(src));  // it was there before, so it stays
    CHECK(ReadFile(dir.path() / ".gitignore") == "build/\n");
}
#endif
