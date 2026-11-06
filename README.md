# Craft

A build tool and project manager for C++, in the spirit of Rust's Cargo.

Craft aims to make C++ development more approachable: you describe your package
in a small `craft.toml`, follow a simple project layout, and `craft build` does
the rest. No CMake, no list of source files, no flags.

> **Status:** early development (Milestone 1). `craft init` and `craft build`
> work. `craft new`, `craft run`, `craft test` and dependencies are planned, see
> the [roadmap](docs/ROADMAP.md).

## Quick start

```sh
mkdir hello && cd hello
craft init
craft build
./target/debug/hello
```

`craft init` creates the project in the current directory, named after it:

```
hello/
├── craft.toml
├── src/
│   └── main.cpp
└── .gitignore
```

`craft build` prints its progress in the style of Cargo:

```
   Compiling hello v0.1.0 (/home/me/hello)
    Finished debug target
```

The program ends up in `target/debug/hello`. For an optimized build, use
`--release`; the result is in `target/release/`.

## Commands

| Command                 | What it does                                          |
|-------------------------|-------------------------------------------------------|
| `craft init`            | Create a new project in the current directory         |
| `craft build`           | Build the project (debug)                             |
| `craft build --release` | Build the project with optimizations                  |
| `craft --help`          | Print help                                            |
| `craft --version`       | Print the version                                     |

You can run `craft build` from any subdirectory of a project: Craft looks for
`craft.toml` in the current directory and then in each parent.

## Project layout

Craft prefers convention over configuration. If you follow the layout below,
nothing else needs to be configured.

```
my-project/
├── craft.toml
├── src/
│   ├── main.cpp        # program, named after the package
│   ├── lib.cppm        # library (lib.cpp or lib.hpp work too)
│   └── ...             # everything else: sources, headers, modules
└── target/             # build output, created by Craft
```

- `src/main.*` is the program, `src/lib.*` is the library.
- Every other source file and module under `src/` belongs to the library, or
  to the program if there is no library.
- With both, the program is linked with the library, so `main.cpp` can
  `import my_project;`. Module names can't contain `-`, so package `my-project`
  exports module `my_project`.
- Hidden files and directories are ignored.

Details: [docs/conventions.md](docs/conventions.md).

## The manifest

```toml
[package]
name = "hello"
version = "0.1.0"
cpp = "23"
```

`name` and `version` are required. `cpp` is the C++ standard (`17`, `20`, `23`
or `26`, default `23`). All fields: [docs/manifest.md](docs/manifest.md).

## Requirements

Craft generates a CMake project in `target/cmake/` and builds it with CMake and
Ninja, so you need:

- CMake 3.28 or newer
- Ninja 1.11 or newer
- a C++ compiler with C++20 modules support: GCC 15+, Clang 19+ or MSVC 17.10+

To pick the compiler, set the `CXX` environment variable:

```sh
CXX=clang++ craft build
```

Changing `CXX` also works in an existing project: Craft passes it to CMake on
every build.

## Building Craft

Craft itself is built with CMake until it can build itself.

```sh
git clone https://github.com/antek042/craft.git
cd craft
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The binary is `build/src/craft`.

## Contributing

Contributions are welcome. Start with [CONTRIBUTING.md](CONTRIBUTING.md); issues
labeled `good first issue` are a good place to begin. The code follows the
[coding style](docs/CODING_STYLE.md) and is tested as described in
[docs/TESTING.md](docs/TESTING.md).

## License

[GNU General Public License v3.0](LICENSE)
