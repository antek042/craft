#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "temp_dir.h"

import craft.build;
import craft.cli;
import craft.core;
import craft.manifest;

namespace {

namespace fs = std::filesystem;

// A build engine that only records what it was asked to do.
class FakeBackend final : public craft::build::Backend {
public:
    std::expected<void, craft::Error> Generate(
        [[maybe_unused]] const craft::manifest::Manifest& manifest,
        const craft::build::Targets& targets,
        const craft::build::BuildOptions& options) override {
        ++generated;
        last_targets = targets;
        last_options = options;
        if (generate_error) {
            return std::unexpected(*generate_error);
        }
        return {};
    }

    std::expected<void, craft::Error> Build(
        [[maybe_unused]] const craft::build::BuildOptions& options) override {
        ++built;
        if (build_error) {
            return std::unexpected(*build_error);
        }
        return {};
    }

    int generated = 0;
    int built = 0;
    craft::build::Targets last_targets;
    craft::build::BuildOptions last_options;
    std::optional<craft::Error> generate_error;
    std::optional<craft::Error> build_error;
};

struct BuildResult {
    int exit_code;
    std::string out;
    std::string err;
};

BuildResult RunBuild(const fs::path& dir,
                     const std::vector<std::string_view>& args,
                     FakeBackend& backend) {
    std::ostringstream out;
    std::ostringstream err;
    const int exit_code =
        craft::cli::BuildProject(dir, args, out, err, backend);
    return {.exit_code = exit_code, .out = out.str(), .err = err.str()};
}

// A project named `hello` with a program (src/main.cpp).
void MakeProject(const fs::path& dir) {
    REQUIRE(craft::cli::CreateProject(dir, "hello").has_value());
}

}  // namespace

TEST_CASE("build generates and builds the project in debug by default",
          "[cli][build]") {
    const TempDir dir;
    MakeProject(dir.path());
    FakeBackend backend;

    const auto result = RunBuild(dir.path(), {}, backend);

    CHECK(result.exit_code == 0);
    CHECK(result.err.empty());
    CHECK(backend.generated == 1);
    CHECK(backend.built == 1);
    CHECK(backend.last_options.profile == craft::build::Profile::kDebug);
    CHECK(backend.last_options.root == dir.path());
    CHECK(backend.last_targets.executable.has_value());
    CHECK_FALSE(backend.last_targets.library.has_value());
}

TEST_CASE("build --release selects the release profile", "[cli][build]") {
    const TempDir dir;
    MakeProject(dir.path());
    FakeBackend backend;

    const auto result = RunBuild(dir.path(), {"--release"}, backend);

    CHECK(result.exit_code == 0);
    CHECK(backend.last_options.profile == craft::build::Profile::kRelease);
    CHECK(result.out.contains("Finished release target"));
}

TEST_CASE("build prints the progress in the style of Cargo", "[cli][build]") {
    const TempDir dir;
    MakeProject(dir.path());
    FakeBackend backend;

    const auto result = RunBuild(dir.path(), {}, backend);

    CHECK(result.out == "   Compiling hello v0.1.0 (" + dir.path().string() +
                            ")\n" + "    Finished debug target\n");
}

TEST_CASE("build finds the project from a subdirectory", "[cli][build]") {
    const TempDir dir;
    MakeProject(dir.path());
    const fs::path nested = dir.path() / "src";
    FakeBackend backend;

    const auto result = RunBuild(nested, {}, backend);

    CHECK(result.exit_code == 0);
    CHECK(backend.last_options.root == dir.path());
}

TEST_CASE("build rejects an unknown flag", "[cli][build]") {
    const TempDir dir;
    MakeProject(dir.path());
    FakeBackend backend;

    const auto result = RunBuild(dir.path(), {"--fast"}, backend);

    CHECK(result.exit_code == 1);
    CHECK(result.out.empty());
    CHECK(result.err.contains("error: unknown flag `--fast`"));
    CHECK(result.err.contains("hint: the only flag is `--release`"));
    CHECK(backend.generated == 0);
    CHECK(backend.built == 0);
}

TEST_CASE("build rejects a stray argument", "[cli][build]") {
    const TempDir dir;
    MakeProject(dir.path());
    FakeBackend backend;

    const auto result = RunBuild(dir.path(), {"extra"}, backend);

    CHECK(result.exit_code == 1);
    CHECK(result.err.contains("takes no arguments, got `extra`"));
    CHECK(backend.built == 0);
}

TEST_CASE("build outside a project fails", "[cli][build]") {
    const TempDir dir;
    FakeBackend backend;

    const auto result = RunBuild(dir.path(), {}, backend);

    CHECK(result.exit_code == 1);
    CHECK(result.err.contains("error:"));
    CHECK(backend.built == 0);
}

TEST_CASE("build without main or lib reports no targets", "[cli][build]") {
    const TempDir dir;
    MakeProject(dir.path());
    fs::remove(dir.path() / "src" / "main.cpp");
    FakeBackend backend;

    const auto result = RunBuild(dir.path(), {}, backend);

    CHECK(result.exit_code == 1);
    CHECK(result.err.contains("error: no targets found"));
    CHECK(result.err.contains("hint: create src/main.cc or src/lib.cppm"));
    CHECK(backend.generated == 0);
}

TEST_CASE("an error from Generate is printed and stops the build",
          "[cli][build]") {
    const TempDir dir;
    MakeProject(dir.path());
    FakeBackend backend;
    backend.generate_error = craft::Error("cannot write the file");

    const auto result = RunBuild(dir.path(), {}, backend);

    CHECK(result.exit_code == 1);
    CHECK(result.err.contains("error: cannot write the file"));
    CHECK(backend.built == 0);
    CHECK_FALSE(result.out.contains("Finished"));
}

TEST_CASE("an error from Build is printed", "[cli][build]") {
    const TempDir dir;
    MakeProject(dir.path());
    FakeBackend backend;
    backend.build_error = craft::Error("cmake build failed (exit code 1)");

    const auto result = RunBuild(dir.path(), {}, backend);

    CHECK(result.exit_code == 1);
    CHECK(result.err.contains("error: cmake build failed (exit code 1)"));
    CHECK(result.out.contains("Compiling"));
    CHECK_FALSE(result.out.contains("Finished"));
}
