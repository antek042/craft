#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "scoped_env.h"
#include "scoped_path.h"
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

#ifndef _WIN32

namespace {

// A directory in PATH with a fake `cmake` that logs its arguments and a fake
// `ninja`. `fail_configure` and `fail_build` files next to the script make
// the matching step exit with 1.
class FakeTools {
public:
    explicit FakeTools(const fs::path& dir, bool with_cmake = true,
                       bool with_ninja = true)
        : bin_(dir / "bin") {
        fs::create_directories(bin_);
        if (with_cmake) {
            WriteScript("cmake",
                        "#!/bin/sh\n"
                        "echo \"$@\" >> \"$0.log\"\n"
                        "case \"$1\" in\n"
                        "  --build) [ -e \"$0.fail_build\" ] && exit 1;;\n"
                        "  *) [ -e \"$0.fail_configure\" ] && exit 1;;\n"
                        "esac\n"
                        "exit 0\n");
        }
        if (with_ninja) {
            WriteScript("ninja", "#!/bin/sh\nexit 0\n");
        }
    }

    [[nodiscard]] const fs::path& bin() const { return bin_; }
    [[nodiscard]] std::string Log() const {
        return ReadFile(bin_ / "cmake.log");
    }
    void Fail(const std::string& step) const {
        std::ofstream(bin_ / ("cmake.fail_" + step)) << "\n";
    }

private:
    void WriteScript(const std::string& name, const std::string& text) const {
        const auto file = bin_ / name;
        std::ofstream(file) << text;
        fs::permissions(file, fs::perms::owner_all);
    }

    fs::path bin_;
};

}  // namespace

TEST_CASE("Build runs cmake configure and then cmake --build",
          "[build][cmake_backend]") {
    const TempDir dir;
    const FakeTools tools(dir.path());
    const ScopedPath path(tools.bin().string());
    craft::build::CMakeBackend backend;
    const auto root = dir.path() / "project";

    const auto result = backend.Build({.root = root});

    REQUIRE(result.has_value());
    const auto log = tools.Log();
    const auto configure = log.find("-S " + (root / "target/cmake").string());
    const auto build = log.find("--build " + (root / "target/debug").string());
    CHECK(configure != std::string::npos);
    CHECK(build != std::string::npos);
    CHECK(configure < build);
    CHECK(log.contains("-B " + (root / "target/debug").string()));
    CHECK(log.contains("-G Ninja"));
    CHECK(log.contains("-DCMAKE_BUILD_TYPE=Debug"));
    CHECK(log.contains("-DCMAKE_MAKE_PROGRAM=" +
                       (tools.bin() / "ninja").string()));
}

TEST_CASE("Build uses target/release for the release profile",
          "[build][cmake_backend]") {
    const TempDir dir;
    const FakeTools tools(dir.path());
    const ScopedPath path(tools.bin().string());
    craft::build::CMakeBackend backend;
    const auto root = dir.path() / "project";

    const auto result = backend.Build(
        {.root = root, .profile = craft::build::Profile::kRelease});

    REQUIRE(result.has_value());
    const auto log = tools.Log();
    CHECK(log.contains("-B " + (root / "target/release").string()));
    CHECK(log.contains("-DCMAKE_BUILD_TYPE=Release"));
    CHECK(log.contains("--build " + (root / "target/release").string()));
}

TEST_CASE("a failing configure step stops the build",
          "[build][cmake_backend]") {
    const TempDir dir;
    const FakeTools tools(dir.path());
    tools.Fail("configure");
    const ScopedPath path(tools.bin().string());
    craft::build::CMakeBackend backend;

    const auto result = backend.Build({.root = dir.path() / "project"});

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("configure"));
    CHECK_FALSE(tools.Log().contains("--build"));
}

TEST_CASE("a failing build step is reported", "[build][cmake_backend]") {
    const TempDir dir;
    const FakeTools tools(dir.path());
    tools.Fail("build");
    const ScopedPath path(tools.bin().string());
    craft::build::CMakeBackend backend;

    const auto result = backend.Build({.root = dir.path() / "project"});

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("build failed"));
}

TEST_CASE("a missing cmake is an error with a hint", "[build][cmake_backend]") {
    const TempDir dir;
    const FakeTools tools(dir.path(), false, true);
    const ScopedPath path(tools.bin().string());
    craft::build::CMakeBackend backend;

    const auto result = backend.Build({.root = dir.path() / "project"});

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("cmake"));
    CHECK(result.error().hint().contains("3.28"));
}

TEST_CASE("a missing ninja is an error with a hint", "[build][cmake_backend]") {
    const TempDir dir;
    const FakeTools tools(dir.path(), true, false);
    const ScopedPath path(tools.bin().string());
    craft::build::CMakeBackend backend;

    const auto result = backend.Build({.root = dir.path() / "project"});

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("ninja"));
    CHECK_FALSE(result.error().hint().empty());
    CHECK(tools.Log().empty());
}

TEST_CASE("Build passes the compiler from CXX to cmake",
          "[build][cmake_backend]") {
    const TempDir dir;
    const FakeTools tools(dir.path());
    const ScopedPath path(tools.bin().string());
    const ScopedEnv cxx("CXX", "my-compiler");
    craft::build::CMakeBackend backend;

    const auto result = backend.Build({.root = dir.path() / "project"});

    REQUIRE(result.has_value());
    CHECK(tools.Log().contains("-DCMAKE_CXX_COMPILER=my-compiler"));
}

TEST_CASE("Build leaves the compiler to cmake when CXX is not set",
          "[build][cmake_backend]") {
    const TempDir dir;
    const FakeTools tools(dir.path());
    const ScopedPath path(tools.bin().string());
    const ScopedEnv cxx("CXX", std::nullopt);
    craft::build::CMakeBackend backend;

    const auto result = backend.Build({.root = dir.path() / "project"});

    REQUIRE(result.has_value());
    CHECK_FALSE(tools.Log().contains("CMAKE_CXX_COMPILER"));
}

#endif
