module;

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

export module craft.manifest:name;

import craft.core;

export namespace craft::manifest {

// Derives a package name from the name of a directory, e.g. `Hello_World`
// gives `hello_world`. Fails when the result is not a valid package name.
//
// `dir` does not have to exist.
[[nodiscard]] std::expected<std::string, Error> PackageNameFromDir(
    const std::filesystem::path& dir);

[[nodiscard]] std::expected<void, Error> ValidatePackageName(
    std::string_view name);

}  // namespace craft::manifest
