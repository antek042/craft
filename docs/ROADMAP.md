# Roadmap

Craft is a build tool and project manager for C++, in the spirit of Rust's Cargo.
This roadmap splits the work into milestones. Each milestone should end with a
usable release, even if small.

## Milestone 0 — Foundations (repository setup)

Goal: anyone can clone, build and test the project in under 5 minutes.

- [x] Project layout (`src/`, `tests/`, `docs/`) — see [Project layout](#project-layout)
- [x] CMake build for Craft itself with C++20 modules (bootstrap until Craft can build itself)
- [x] Test framework (Catch2 v3) wired into CTest
- [x] `.clang-format`, `.clang-tidy`, `.editorconfig`
- [x] CI: build + tests on Linux, macOS, Windows (GCC, Clang, MSVC)
- [x] CI: format check, clang-tidy, sanitizers (ASan + UBSan)
- [ ] `CONTRIBUTING.md`, issue and PR templates, Code of Conduct
- [ ] `good first issue` and `help wanted` labels

## Milestone 1 — MVP: single-package projects

Goal: a newcomer can create, build, run and test a project without writing CMake.

- [ ] Manifest format `craft.toml` (name, version, C++ standard, targets)
- [ ] `craft new <name>` / `craft init` — create a project skeleton
- [ ] `craft build` — compile using the system compiler (debug / `--release`)
- [ ] `craft run` — build and run the binary
- [ ] `craft test` — build and run tests
- [ ] `craft clean`
- [ ] Clear, friendly error messages (compiler not found, bad manifest, etc.)
- [ ] Incremental builds (only rebuild changed files, track header and module dependencies)
- [ ] Support for C++20 modules in user projects (dependency scanning, build order)

## Milestone 2 — Dependencies

- [ ] Dependencies from git (`url` + `tag`/`rev`) and local paths
- [ ] Lock file `craft.lock` for reproducible builds
- [ ] Building CMake-based dependencies (huge part of today's ecosystem)
- [ ] Dependency cache in the user's home directory

## Milestone 3 — Ecosystem and comfort

- [ ] Workspaces (many packages in one repository)
- [ ] `craft fmt` and `craft lint` (wrappers for clang-format / clang-tidy)
- [ ] `compile_commands.json` generation for editors / clangd
- [ ] Self-hosting: Craft builds itself with Craft
- [ ] Package registry (design discussion first — RFC)

## Out of scope (for now)

- Replacing CMake for existing large projects
- Supporting compilers older than our minimum (see `docs/CODING_STYLE.md`)

## Project layout

```
craft/
├── CMakeLists.txt
├── src/
│   ├── main.cc             # thin entry point, only parses args and calls into lib
│   └── craft/              # library code, one C++ module per directory
│       ├── core/           # module craft.core — basic types (Error, version)
│       ├── cli/            # module craft.cli — command-line parsing and commands
│       ├── manifest/       # module craft.manifest — craft.toml parsing and validation
│       ├── build/          # module craft.build — build graph, compiler invocation
│       ├── util/           # module craft.util — small shared helpers (fs, process, strings)
│       └── macros/         # the only headers: macros like CRAFT_CHECK
├── tests/
│   ├── unit/               # fast tests of single components
│   ├── integration/        # run the real `craft` binary on fixture projects
│   └── fixtures/           # sample projects used by integration tests
└── docs/
```

## How decisions are made

Bigger changes (new manifest fields, new commands, new dependencies of Craft
itself) start as a GitHub Discussion or an issue labeled `rfc`. Code comes after
the design is agreed on — this saves contributors from wasted work.
