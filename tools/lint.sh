#!/usr/bin/env bash
# Checks formatting and runs clang-tidy, the same way CI does.
#
# Usage: tools/lint.sh [build-dir]    (default build dir: build-lint)
set -euo pipefail

cd "$(dirname "$0")/.."
build_dir="${1:-build-lint}"
mapfile -t files < <(git ls-files '*.cc' '*.cppm')

echo "== clang-format =="
clang-format --dry-run --Werror "${files[@]}"
echo "ok"

echo "== clang-tidy =="
# clang-tidy reads imported modules from the build directory, so the
# project has to be built (with Clang) first.
cmake -B "$build_dir" -G Ninja -DCMAKE_CXX_COMPILER=clang++ > /dev/null
cmake --build "$build_dir"
clang-tidy --quiet -p "$build_dir" "${files[@]}"
echo "ok"
