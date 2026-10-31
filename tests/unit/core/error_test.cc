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
