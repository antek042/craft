module;

#include <expected>
#include <filesystem>

export module craft.manifest:loader;

import craft.core;

export namespace craft::manifest {

// Looks for `craft.toml` in `start` and then in each parent directory.
// `start` can be a directory or a file inside the project.
//
// Returns the path of the first manifest found.
[[nodiscard]] std::expected<std::filesystem::path, Error> FindManifest(
    const std::filesystem::path& start);

}  // namespace craft::manifest
