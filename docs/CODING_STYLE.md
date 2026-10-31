# Coding style

Craft follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
This document lists the most important rules, our deviations, and the things
specific to Craft. If something isn't covered here, the Google guide decides.

Formatting is handled by `clang-format` (`BasedOnStyle: Google` with our changes)
and many rules are checked by `clang-tidy`. Don't argue with the formatter — run
it and move on.

## Deviations from the Google guide

| Topic          | Google guide              | Craft                                      |
|----------------|---------------------------|--------------------------------------------|
| C++ standard   | C++20                     | **C++23** (we need `std::expected`)         |
| Indentation    | 2 spaces                  | **4 spaces**                               |
| Code organization | headers (`.h`) + sources (`.cc`) | **C++20 modules** (see [Modules](#modules)) |

Everything else follows the guide as written.

## Language and compilers

- **C++23 with modules.** Minimum: GCC 15, Clang 19, MSVC 17.10 (Visual Studio 2022),
  CMake 3.28 and Ninja 1.11 (the Makefile generator doesn't support modules).
- Use only features that work on all three compilers. If unsure, check
  [cppreference compiler support](https://en.cppreference.com/w/cpp/compiler_support).
- Warnings are errors in CI (`-Wall -Wextra -Wpedantic -Werror`, `/W4 /WX`).
- Craft itself should have as few third-party dependencies as possible. Adding one
  needs an issue and maintainer approval.

## Formatting (enforced by clang-format)

- 4-space indentation, no tabs.
- Line length: 80 characters.
- Opening braces on the same line.

## Modules

All Craft code is organized in C++20 named modules. There are no project header files.

### Structure

- **One module per directory** under `src/craft/`. The module name follows the path:
  `src/craft/manifest/` → module `craft.manifest`.
- Bigger modules are split into **partitions**, one per file:
  `src/craft/manifest/parser.cppm` → `craft.manifest:parser`.
- Each module has a **primary interface** file named after the directory
  (`manifest.cppm`), which re-exports the public partitions:
  ```cpp
  export module craft.manifest;

  export import :parser;
  export import :manifest;
  ```
- Code that is only an implementation detail goes into a non-exported partition
  or a module implementation unit (`.cc`), so it doesn't leak to users of the module.

### File extensions

| File                                        | Extension | Example              |
|---------------------------------------------|-----------|----------------------|
| Module interface unit / interface partition | `.cppm`   | `parser.cppm`        |
| Module implementation unit                  | `.cc`     | `parser.cc`          |
| Program entry point                         | `.cc`     | `main.cc`            |
| Test                                        | `_test.cc`| `parser_test.cc`     |

Files are listed in CMake in `FILE_SET CXX_MODULES` (interfaces) or as normal
sources (implementation units).

### Layout of a module file

```cpp
module;  // global module fragment — only #include of non-module code goes here

#include <expected>
#include <string>
#include <string_view>

export module craft.manifest:parser;

import craft.util;
import :error;

export namespace craft::manifest {

// Parses the contents of a `craft.toml` file.
[[nodiscard]] std::expected<Manifest, Error> Parse(std::string_view text);

}  // namespace craft::manifest
```

Rules:

- **Standard library and third-party libraries** are included with `#include`
  in the global module fragment (`module;`), in Google order: C system headers,
  C++ standard library, other libraries. We'll switch to `import std;` once
  CMake and all supported compilers handle it reliably.
- **Craft code** is always used with `import`, never `#include`.
- Order after the module declaration: other Craft modules (`import craft.util;`),
  then partitions of the same module (`import :error;`).
- Export explicitly. Prefer an `export namespace` block or `export` on single
  declarations; don't export helpers that users of the module don't need.
- Macros don't cross module boundaries. If something really needs a macro
  (e.g. `CRAFT_CHECK`), it lives in a small header in `src/craft/macros/`
  — the only project headers allowed.
- Avoid cyclic dependencies between modules — the build won't allow them anyway.
  If two modules need each other, the shared part becomes a third module.

## Naming

| Thing                                | Style                       | Example                         |
|--------------------------------------|-----------------------------|---------------------------------|
| Modules                              | `lowercase.dotted`          | `craft.manifest`                |
| Module partitions                    | `snake_case`                | `craft.manifest:toml_reader`    |
| Types, classes, structs, enums, concepts | `PascalCase`            | `BuildGraph`, `Manifest`        |
| Functions, methods                   | `PascalCase`                | `ParseManifest()`, `AddTarget()`|
| Accessors / mutators (optional)      | `snake_case`                | `name()`, `set_name()`          |
| Variables, parameters                | `snake_case`                | `target_name`                   |
| Class data members                   | `snake_case_` (trailing `_`)| `targets_`                      |
| Struct data members                  | `snake_case`                | `version`                       |
| Constants (`constexpr`, `const` static) | `kPascalCase`            | `kDefaultJobs`                  |
| Enumerators                          | `kPascalCase`               | `Profile::kRelease`             |
| Macros (avoid them)                  | `CRAFT_UPPER_CASE`          | `CRAFT_CHECK`                   |
| Namespaces                           | `snake_case`                | `craft::manifest`               |
| Files                                | `snake_case`                | `build_graph.cppm`, `build_graph.cc` |

Namespaces match module names: module `craft.manifest` exports into
`namespace craft::manifest`.

Names should say what something is. Avoid abbreviations unless they are widely
known (`id`, `url`, `fs`).

## Error handling

Craft is a tool used by people — **errors are part of the user interface.**

- As in the Google guide, **we don't use exceptions.** Code that can fail returns
  `std::expected<T, craft::Error>`.
- `craft::Error` carries a message for the user and, if possible, a hint on how
  to fix it:
  ```
  error: could not find `craft.toml` in `/home/user/app` or any parent directory
  hint: run `craft init` to create a new project here
  ```
- Programmer errors (broken invariants) use `CRAFT_CHECK`, which aborts with a
  message. They are bugs in Craft, not user errors.

## General rules

- Prefer value types, references and `std::unique_ptr`. No raw `new`/`delete`.
  Use `std::shared_ptr` only if ownership is really shared.
- Use `std::string_view` and `std::span` for read-only parameters.
- Mark things `const` and `[[nodiscard]]` where it makes sense
  (always for functions returning `std::expected`).
- Output parameters are avoided — return values (or a struct) instead.
- Use `std::filesystem::path` for paths — never build them by concatenating strings.
  Craft must work on Windows.
- Keep functions short (Google suggests ~40 lines as a signal to split).
- No global mutable state and no static objects with non-trivial destructors.
- Platform-specific code lives in the `craft.util.platform` module behind a common
  interface, not as `#ifdef` scattered across the code.
- No `using namespace` directives anywhere. `using` declarations and aliases are
  fine inside implementation units and function bodies.

## Comments

- Comment **why**, not **what**. The code already says what.
- Every exported class and every non-obvious exported function gets a comment
  describing what it does and how to use it (Google style, `//` comments).
- `TODO` comments link an issue: `// TODO(#42): Support workspaces.`
