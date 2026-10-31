# The `craft.toml` manifest

Every Craft project has a `craft.toml` file in its root directory. It describes
the package: its name, version and which C++ standard it uses.

The file uses [TOML](https://toml.io), a simple configuration format.

> **Status:** this describes the format planned for Milestone 1. Sections marked
> *planned* are not supported yet.

## Example

```toml
[package]
name = "hello"
version = "0.1.0"
cpp = "23"
description = "Prints a friendly greeting"
license = "MIT"
```

## Finding the manifest

Craft looks for `craft.toml` in the current directory, then in each parent
directory, and uses the first one it finds. That directory is the project root.
So you can run `craft build` from any subdirectory of your project.

## The `[package]` section

Required. Describes the package.

| Key           | Required | Type   | Default | Description                      |
|---------------|----------|--------|---------|----------------------------------|
| `name`        | yes      | string | —       | Package name                     |
| `version`     | yes      | string | —       | Package version                  |
| `cpp`         | no       | string | `"23"`  | C++ standard used to compile it  |
| `description` | no       | string | —       | One-line description             |
| `license`     | no       | string | —       | License, as an [SPDX identifier](https://spdx.org/licenses/) |

### `name`

- Only lowercase letters `a-z`, digits `0-9`, `-` and `_`.
- Starts with a letter.
- At most 64 characters.

Valid: `hello`, `my-app`, `json_parser2`. Invalid: `Hello`, `2fast`, `my app`.

### `version`

A [semantic version](https://semver.org): three numbers separated by dots,
`MAJOR.MINOR.PATCH`, e.g. `0.1.0` or `2.13.4`. A pre-release suffix is allowed:
`1.0.0-beta.2`. Numbers must not have leading zeros (`1.02.0` is invalid).

### `cpp`

The C++ standard: one of `"17"`, `"20"`, `"23"`, `"26"`. It is a string, not a
number, so `cpp = 23` is an error with a hint to write `cpp = "23"`.

## Targets

Craft finds targets by convention, so there is nothing to configure:

| File               | Target                                   |
|--------------------|------------------------------------------|
| `src/main.cc`      | executable named after the package       |
| `src/lib.cppm`     | library (a C++ module named after the package) |

A package can have both. If neither exists, `craft build` reports an error.

Module names can't contain `-`, so in the library's module name dashes become
underscores: package `my-lib` exports module `my_lib`.

*Planned:* `[[bin]]` and `[lib]` sections to override these defaults.

## Planned sections

- `[dependencies]` — packages from git or local paths (Milestone 2)
- `[[bin]]`, `[lib]` — explicit targets
- `[workspace]` — many packages in one repository (Milestone 3)

## Errors

Craft checks the whole manifest before building. Every error points to the
place in the file and, where possible, says how to fix it:

```
error: invalid version "1.0"
  --> craft.toml:3:11
hint: versions have three numbers, e.g. "1.0.0"
```

**Unknown keys and sections are errors**, not warnings, so typos don't go
unnoticed:

```
error: unknown key "nmae" in [package]
  --> craft.toml:2:1
hint: did you mean "name"?
```

Sections that are planned but not supported yet (like `[dependencies]` today)
are also errors, with a hint that they are coming.

## For contributors

The manifest is implemented in the `craft.manifest` module
(`src/craft/manifest/`) in two layers:

1. **TOML** — [toml++](https://github.com/marzer/tomlplusplus) turns the text
   into tables and values, with the line and column of each value.
2. **Manifest** — `craft::manifest::Parse(text, path)` turns those tables into
   a `Manifest` struct and checks the rules above. It returns
   `std::expected<Manifest, craft::Error>`.

Only `:parser` talks to toml++, so the TOML library can be replaced without
touching the rest. `Parse` takes text, not a file, so most tests don't need
the filesystem.

```
src/craft/manifest/
├── manifest.cppm      # primary interface, re-exports the partitions
├── types.cppm         # :types    — Manifest, Package, CppStandard
├── parser.cppm/.cc    # :parser   — Parse(text, path)
├── validate.cppm/.cc  # :validate — one function per rule
└── loader.cppm/.cc    # :loader   — FindManifest(dir), Load(path)
```

When you change the format, update this document in the same pull request.
