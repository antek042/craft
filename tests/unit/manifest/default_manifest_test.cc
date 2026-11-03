#include <expected>
#include <string>

#include <catch2/catch_test_macros.hpp>

import craft.core;
import craft.manifest;

TEST_CASE("default manifest can be parsed", "[manifest]") {
    const auto result = craft::manifest::Parse(
        craft::manifest::DefaultManifest("hello"), "craft.toml");

    REQUIRE(result.has_value());
    CHECK(result->package.name == "hello");
    CHECK(result->package.version == "0.1.0");
    CHECK(result->package.cpp_standard == craft::manifest::CppStandard::k23);
}

TEST_CASE("default manifest contains the package name", "[manifest]") {
    const std::string text = craft::manifest::DefaultManifest("my-app");

    CHECK(text.contains("name = \"my-app\"\n"));
}

TEST_CASE("default manifest works for every valid name", "[manifest]") {
    for (const std::string name : {"a", "my-app", "json_parser2"}) {
        const auto result = craft::manifest::Parse(
            craft::manifest::DefaultManifest(name), "craft.toml");

        REQUIRE(result.has_value());
        CHECK(result->package.name == name);
    }
}
