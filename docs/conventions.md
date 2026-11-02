# Project conventions

Craft prefers **convention over configuration**. If you follow the layout on
this page, `craft build` works without a single line of build configuration:
no CMake, no list of source files, no flags.

Two names to remember: **`main` is a program, `lib` is a library.**

## Layout

```
hello/
├── craft.toml
├── src/
│   ├── main.cpp        # program (named after the package)
│   ├── lib.cppm        # library (lib.cpp or lib.hpp work too)
│   ├── bin/            # more programs: src/bin/tool.cpp -> `tool`
│   └── ...             # everything else: sources, headers, modules, subdirectories
├── include/            # optional: public headers (see "Headers")
├── tests/              # one program per file, run by `craft test`
└── target/             # build output, created by Craft
```

`craft init` creates `craft.toml`, `src/main.cpp` and a `.gitignore` that
ignores `target/`.

## The rules

1. **`src/main.*` is a program, `src/lib.*` is a library.** A package can have
   either or both. With neither, `craft build` reports an error that says to
   create one of them.
2. **Everything else in `src/`** (including subdirectories) belongs to the
   library. If there is no library, it belongs to the program.
3. **`src/bin/*.*` are extra programs.** Each file is one program named after
   the file. They link the library if there is one.
4. **`tests/*.*` are test programs.** Each file is one program, linked with the
   library if there is one. A test passes when it exits with code 0.
5. **File extensions don't decide the role, only the kind of file:**

   | Kind | Extensions |
   |------|------------|
   | Sources | `.cpp` `.cc` `.cxx` |
   | Headers | `.h` `.hpp` `.hh` `.hxx` |
   | Module interfaces | `.cppm` `.ixx` |

   So `main.cpp`, `main.cc` and `main.cxx` mean the same thing. Having two of
   them (`main.cpp` and `main.cc`) is an error, because Craft doesn't guess.
6. **Hidden files and directories** (names starting with `.`) and `target/` are
   ignored.

## Headers

Headers and modules can be mixed freely in one project.

- **`src/` is the include directory**, so `#include "util.h"` works from every
  file in the project, in any subdirectory.
- **`include/` is optional.** If it exists, it holds the library's *public*
  headers and is what other packages see; `src/` then stays private.
  Without `include/`, the headers in `src/` are public.
- Header files don't need to be listed anywhere.

## Modules

- A library whose entry point is `src/lib.cppm` exports the module named after
  the package. Module names can't contain `-`, so package `my-lib` exports
  module `my_lib`.
- Module interface files (`.cppm`, `.ixx`) are found like any other file. You
  never list them, and Craft works out the build order itself.
- A project without any module interface files is built without module
  scanning, so it needs no particularly new tools.

## Examples

**A small program**

```
src/main.cpp
src/util.cpp
src/util.h
```
One program `hello`. Without `lib.*`, `util.cpp` belongs to the program.

**A library with a program that uses it**

```
src/lib.cppm      # export module hello;
src/main.cpp      # import hello;
```
A library `hello` and a program `hello` linked with it.

**A header-only library**

```
src/lib.hpp
include/...       # optional
```
A library without compiled sources.

**More than one program**

```
src/lib.cpp
src/main.cpp
src/bin/migrate.cpp
src/bin/seed.cpp
```
The library and three programs: `hello`, `migrate` and `seed`.

## When the convention doesn't fit

The manifest can override any default, for example `[[bin]]` for a program in
a different place (planned, see [manifest.md](manifest.md)). Overrides are
meant to be rare: if you need them often, we want to hear about it, because
the convention should probably change.

## For contributors

The rules above are implemented by the target discovery in `craft.build`, which
returns a list of targets from the manifest and the file system. The
`CMakeLists.txt` generator only reads that list; it never looks at the file
system. Keep it that way, and update this page in the same pull request when
you change a rule.
