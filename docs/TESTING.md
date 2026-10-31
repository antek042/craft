# Testing

Every change in behavior comes with a test. Bug fixes come with a test that fails
before the fix.

## Tools

- **Catch2 v3** for all C++ tests, fetched by CMake (`FetchContent`).
- **CTest** to run everything: `ctest --test-dir build --output-on-failure`.
- **Sanitizers** in CI: AddressSanitizer + UndefinedBehaviorSanitizer on Linux/Clang.
- **Coverage** is reported in CI as information, not as a hard gate.

## Kinds of tests

### Unit tests — `tests/unit/`

Test one component (manifest parser, build graph, version comparison) in isolation.

- Fast: the whole unit suite should run in seconds.
- No network. Filesystem only inside a temporary directory.
- One test file per module partition: `src/craft/manifest/parser.cppm` →
  `tests/unit/manifest/parser_test.cc`.
- Tests `import` the module they test, so they only see its exported interface.

### Integration tests — `tests/integration/`

Run the real `craft` binary on sample projects from `tests/fixtures/`, like a user would.
Check exit code, output and created files.

- Each test copies its fixture into a fresh temporary directory.
- Compare output to expected text, but ignore paths and timings that change
  between machines.
- Every user-visible error message should have at least one integration test.

## Writing tests

```cpp
#include <catch2/catch_test_macros.hpp>

import craft.manifest;

TEST_CASE("manifest without [package] section is rejected", "[manifest]") {
    auto result = craft::manifest::Parse(R"(
        [dependencies]
        fmt = "11.0"
    )");

    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().message().contains("[package]"));
}
```

Rules:

- Test name is a sentence describing behavior, not the function name.
  Don't start it with `-` — Catch2 would read it as a command-line option.
- Arrange / act / assert — keep these three parts visible.
- One behavior per `TEST_CASE`. Use `SECTION` for variants of the same setup.
- Tag tests by area (`[manifest]`, `[build]`, `[cli]`) so they can be run selectively.
- Tests must be deterministic and independent of order. No `sleep`.
- Don't test private details — test through the public interface.

## Running tests

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure

# only manifest tests
./build/tests/craft_unit_tests "[manifest]"

# with sanitizers
cmake -B build-asan -G Ninja -DCRAFT_SANITIZE=ON
```
