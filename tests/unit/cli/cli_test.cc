#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

import craft.cli;
import craft.core;

namespace {

struct RunResult {
    int exit_code;
    std::string out;
    std::string err;
};

RunResult RunCraft(std::vector<std::string_view> args) {
    std::ostringstream out;
    std::ostringstream err;
    const int exit_code = craft::cli::Run(args, out, err);
    return {.exit_code = exit_code, .out = out.str(), .err = err.str()};
}

}  // namespace

TEST_CASE("running without arguments prints usage", "[cli]") {
    const RunResult result = RunCraft({});

    CHECK(result.exit_code == 0);
    CHECK(result.out.contains("Usage: craft"));
    CHECK(result.err.empty());
}

TEST_CASE("help flags print usage", "[cli]") {
    for (const std::string_view flag : {"--help", "-h"}) {
        const RunResult result = RunCraft({flag});

        CHECK(result.exit_code == 0);
        CHECK(result.out.contains("Usage: craft"));
    }
}

TEST_CASE("version flags print the version", "[cli]") {
    for (const std::string_view flag : {"--version", "-V"}) {
        const RunResult result = RunCraft({flag});

        CHECK(result.exit_code == 0);
        CHECK(result.out == "craft " + std::string(craft::kVersion) + "\n");
    }
}

TEST_CASE("unknown command fails with a hint", "[cli]") {
    const RunResult result = RunCraft({"frobnicate"});

    CHECK(result.exit_code == 1);
    CHECK(result.out.empty());
    CHECK(result.err.contains("error: unknown command `frobnicate`"));
    CHECK(result.err.contains("hint: run `craft --help`"));
}
