# Contributing to Craft

Thanks for your interest! Craft aims to make C++ more approachable, and that
includes contributing to Craft itself. Newcomers are welcome — if something in
this guide is unclear, that's a bug: please open an issue.

## Where to start

- Read the [roadmap](docs/ROADMAP.md) to see what we're building.
- Look for issues labeled [`good first issue`](../../labels/good%20first%20issue).
- Before starting bigger work, comment on the issue (or open one) so nobody
  duplicates effort. Changes to the manifest format or CLI need agreement first.

## Setting up

Craft uses C++20 modules, so tool versions matter. Requirements:

- CMake 3.28+
- Ninja 1.11+ (required — the Makefile generator doesn't support modules)
- one of: GCC 15+, Clang 19+, MSVC 17.10+ (Visual Studio 2022)

```sh
git clone https://github.com/<you>/craft.git
cd craft
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Recommended: clangd 19+ in your editor (CMake generates `compile_commands.json`).
Module support in clangd is still experimental — start it with
`--experimental-modules-support`.

## Workflow

1. Fork the repo and create a branch: `feat/craft-run`, `fix/manifest-crash`.
2. Make your change, with tests (see [Testing](docs/TESTING.md)).
3. Follow the [coding style](docs/CODING_STYLE.md) and run the formatter:
   ```sh
   git clang-format main
   ```
4. Make sure everything builds without warnings and all tests pass, then run
   the same format and clang-tidy checks as CI (needs Clang and
   `clang-tools-extra`):
   ```sh
   tools/lint.sh
   ```
5. Open a pull request. Fill in the template and link the issue (`Closes #12`).

## Commit messages

We use [Conventional Commits](https://www.conventionalcommits.org/):

```
feat(cli): add `craft run` command
fix(manifest): report line number for invalid version
docs: explain dependency resolution
test(build): cover incremental rebuild after header change
```

Types: `feat`, `fix`, `docs`, `test`, `refactor`, `perf`, `build`, `ci`, `chore`.
Keep the first line under 72 characters. Explain *why* in the body if it isn't obvious.

Sign off your commits (`git commit -s`) to certify the
[Developer Certificate of Origin](https://developercertificate.org/).

## Pull requests

- One topic per PR. Small PRs are reviewed faster.
- CI must be green: build on all platforms, tests, format, clang-tidy, sanitizers.
- At least one maintainer approval is needed to merge. PRs are squash-merged,
  so the PR title should be a good Conventional Commit message.
- Reviews are about the code, not the person. Be kind, be specific, suggest fixes.

## Reporting bugs

Include: Craft version (`craft --version`), OS, compiler and version, the
`craft.toml` (or a minimal one that reproduces the bug), the command you ran,
and the full output.

## License

Craft is licensed under GPL-3.0. By contributing you agree that your
contributions are licensed under the same terms.
