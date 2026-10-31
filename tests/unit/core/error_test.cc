#include <catch2/catch_test_macros.hpp>

import craft.core;

TEST_CASE("error without a hint is printed on one line", "[core]") {
    const craft::Error error("could not find `craft.toml`");

    CHECK(error.Format() == "error: could not find `craft.toml`\n");
}

TEST_CASE("error with a hint prints the hint on its own line", "[core]") {
    const craft::Error error("could not find `craft.toml`",
                             "run `craft init` to create a new project here");

    CHECK(error.Format() ==
          "error: could not find `craft.toml`\n"
          "hint: run `craft init` to create a new project here\n");
}

TEST_CASE("error with a location prints it between message and hint",
          "[core]") {
    const craft::Error error("invalid version \"1.0\"",
                             "versions have three numbers, e.g. \"1.0.0\"",
                             craft::ErrorLocation("craft.toml", 3, 11));

    CHECK(error.Format() ==
          "error: invalid version \"1.0\"\n"
          "  --> craft.toml:3:11\n"
          "hint: versions have three numbers, e.g. \"1.0.0\"\n");
}

TEST_CASE("error with a location but no hint omits the hint line", "[core]") {
    const craft::Error error("unexpected character", {},
                             craft::ErrorLocation("craft.toml", 1, 5));

    CHECK(error.Format() ==
          "error: unexpected character\n"
          "  --> craft.toml:1:5\n");
}
