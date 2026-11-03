#include <expected>
#include <filesystem>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

import craft.core;
import craft.manifest;

namespace {

namespace fs = std::filesystem;

}  // namespace

TEST_CASE("valid package names are accepted", "[manifest][name]") {
    const std::string name =
        GENERATE(as<std::string>{}, "hello", "my-app", "json_parser2", "a",
                 "x-1_y", std::string(64, 'a'));

    CHECK(craft::manifest::ValidatePackageName(name).has_value());
}

TEST_CASE("invalid package names are rejected with a reason",
          "[manifest][name]") {
    const auto [name, message] = GENERATE(table<std::string, std::string>({
        {"", "package name is empty"},
        {std::string(65, 'a'), "is too long (65 characters, at most 64)"},
        {"my app", "contains an invalid character ' '"},
        {"my.app", "contains an invalid character '.'"},
        {"Hello", "must be lowercase"},
        {"2fast", "must start with a letter"},
        {"-app", "must start with a letter"},
        {"_app", "must start with a letter"},
    }));

    const auto result = craft::manifest::ValidatePackageName(name);

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains(message));
    CHECK_FALSE(result.error().hint().empty());
}

TEST_CASE("uppercase names get a lowercase suggestion", "[manifest][name]") {
    const auto result = craft::manifest::ValidatePackageName("HelloWorld");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().hint() == "use lowercase, e.g. \"helloworld\"");
}

TEST_CASE("name is taken from the directory name", "[manifest][name]") {
    const auto result =
        craft::manifest::PackageNameFromDir(fs::path("/projects") / "my-app");

    REQUIRE(result.has_value());
    CHECK(*result == "my-app");
}

TEST_CASE("directory name is converted to lowercase", "[manifest][name]") {
    const auto result = craft::manifest::PackageNameFromDir(
        fs::path("/projects") / "Hello_World");

    REQUIRE(result.has_value());
    CHECK(*result == "hello_world");
}

TEST_CASE("trailing separators and dots do not hide the directory name",
          "[manifest][name]") {
    const fs::path base = fs::path("/projects") / "my-app";

    for (const fs::path& dir :
         {base / "", base / ".", base / "." / "", base / "sub" / ".."}) {
        const auto result = craft::manifest::PackageNameFromDir(dir);

        REQUIRE(result.has_value());
        CHECK(*result == "my-app");
    }
}

TEST_CASE("directory that gives no valid name is rejected",
          "[manifest][name]") {
    const auto result =
        craft::manifest::PackageNameFromDir(fs::path("/projects") / "My App");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().starts_with(
        "cannot derive a package name from directory `My App`"));
    CHECK(result.error().hint().contains("craft.toml"));
}

TEST_CASE("name starting with a digit is rejected", "[manifest][name]") {
    const auto result =
        craft::manifest::PackageNameFromDir(fs::path("/projects") / "2fast");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("must start with a letter"));
}

TEST_CASE("the root directory has no package name", "[manifest][name]") {
    const auto result =
        craft::manifest::PackageNameFromDir(fs::current_path().root_path());

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().starts_with(
        "cannot derive a package name from directory"));
}
