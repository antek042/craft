#include <expected>
#include <string>

#include <catch2/catch_test_macros.hpp>

import craft.core;
import craft.manifest;

namespace {

// Parses `text` as if it came from a file named craft.toml.
std::expected<craft::manifest::Manifest, craft::Error> ParseText(
    const std::string& text) {
    return craft::manifest::Parse(text, "craft.toml");
}

}  // namespace

TEST_CASE("valid manifest is parsed", "[manifest]") {
    const auto result = ParseText(
        "[package]\n"
        "name = \"hello\"\n"
        "version = \"0.1.0\"\n");

    REQUIRE(result.has_value());
    CHECK(result->package.name == "hello");
    CHECK(result->package.version == "0.1.0");
}

TEST_CASE("cpp standard defaults to C++23", "[manifest]") {
    const auto result = ParseText(
        "[package]\n"
        "name = \"hello\"\n"
        "version = \"0.1.0\"\n");

    REQUIRE(result.has_value());
    CHECK(result->package.cpp_standard == craft::manifest::CppStandard::k23);
}

TEST_CASE("invalid TOML is reported with its location", "[manifest]") {
    const auto result = ParseText("[package\n");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().Format().contains("--> craft.toml:1:"));
}

TEST_CASE("manifest without [package] is rejected", "[manifest]") {
    const auto result = ParseText("[other]\nkey = 1\n");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "missing [package] section");
    CHECK(result.error().hint().contains("[package]"));
}

TEST_CASE("package that is not a section is rejected", "[manifest]") {
    const auto result = ParseText("package = 5\n");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "`package` must be a section");
    CHECK(result.error().Format().contains("--> craft.toml:1:"));
}

TEST_CASE("missing name is rejected", "[manifest]") {
    const auto result = ParseText("[package]\nversion = \"0.1.0\"\n");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "missing key `name` in [package]");
}

TEST_CASE("missing version is rejected", "[manifest]") {
    const auto result = ParseText("[package]\nname = \"hello\"\n");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "missing key `version` in [package]");
}

TEST_CASE("name of the wrong type points at the value", "[manifest]") {
    const auto result = ParseText(
        "[package]\n"
        "name = 5\n"
        "version = \"0.1.0\"\n");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "`name` must be a string");
    CHECK(result.error().Format().contains("--> craft.toml:2:8"));
}

TEST_CASE("version of the wrong type is rejected", "[manifest]") {
    const auto result = ParseText(
        "[package]\n"
        "name = \"hello\"\n"
        "version = 1.0\n");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message() == "`version` must be a string");
}
