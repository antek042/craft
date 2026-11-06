#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

import craft.util;

namespace {

using craft::util::ProcessOptions;

// A command that runs `script` in the shell of the system.
std::vector<std::string> Shell(const std::string& script) {
#ifdef _WIN32
    return {"cmd.exe", "/c", script};
#else
    return {"/bin/sh", "-c", script};
#endif
}

constexpr ProcessOptions kCapture{.capture_output = true};

}  // namespace

TEST_CASE("RunProcess returns the exit code 0", "[util][run_process]") {
    const auto result = craft::util::RunProcess(Shell("exit 0"));

    REQUIRE(result.has_value());
    CHECK(result->exit_code == 0);
}

TEST_CASE("a non-zero exit code is a result, not an error",
          "[util][run_process]") {
    const auto result = craft::util::RunProcess(Shell("exit 3"));

    REQUIRE(result.has_value());
    CHECK(result->exit_code == 3);
}

TEST_CASE("a missing program is an error", "[util][run_process]") {
    const auto result =
        craft::util::RunProcess({"/craft_test/no_such_program"});

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("no_such_program"));
}

TEST_CASE("an empty command is an error", "[util][run_process]") {
    const auto result = craft::util::RunProcess({});

    CHECK_FALSE(result.has_value());
}

TEST_CASE("RunProcess collects stdout when asked", "[util][run_process]") {
    const auto result = craft::util::RunProcess(Shell("echo hello"), kCapture);

    REQUIRE(result.has_value());
    CHECK(result->exit_code == 0);
    CHECK(result->output.contains("hello"));
}

TEST_CASE("RunProcess collects stderr too", "[util][run_process]") {
    const auto result =
        craft::util::RunProcess(Shell("echo oops 1>&2"), kCapture);

    REQUIRE(result.has_value());
    CHECK(result->output.contains("oops"));
}

TEST_CASE("output is not collected by default", "[util][run_process]") {
    const auto result = craft::util::RunProcess(Shell("exit 0"));

    REQUIRE(result.has_value());
    CHECK(result->output.empty());
}

#ifndef _WIN32

TEST_CASE("arguments are passed as they are, without a shell",
          "[util][run_process]") {
    const auto result = craft::util::RunProcess(
        {"/bin/sh", "-c", "printf '%s' \"$1\"", "sh", "a b;$HOME \"q\""},
        kCapture);

    REQUIRE(result.has_value());
    CHECK(result->output == "a b;$HOME \"q\"");
}

TEST_CASE("a program killed by a signal gives 128 + the signal",
          "[util][run_process]") {
    const auto result = craft::util::RunProcess(Shell("kill -9 $$"));

    REQUIRE(result.has_value());
    CHECK(result->exit_code == 128 + 9);
}

TEST_CASE("a lot of output does not block", "[util][run_process]") {
    const auto result = craft::util::RunProcess(
        Shell("head -c 2000000 /dev/zero | tr '\\0' 'a'"), kCapture);

    REQUIRE(result.has_value());
    CHECK(result->output.size() == 2000000);
}

#endif
