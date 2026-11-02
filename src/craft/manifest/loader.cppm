module;

#include <expected>
#include <filesystem>

export module craft.manifest:loader;

import craft.core;
import :types;

export namespace craft::manifest {

// Looks for `craft.toml` in `start` and then in each parent directory.
// `start` can be a directory or a file inside the project.
//
// Returns the path of the first manifest found.
[[nodiscard]] std::expected<std::filesystem::path, Error> FindManifest(
    const std::filesystem::path& start);

// Reads the manifest at `manifest_path` (usually found by FindManifest) and
// parses it with Parse().
[[nodiscard]] std::expected<Manifest, Error> LoadManifest(
    const std::filesystem::path& manifest_path);

}  // namespace craft::manifest
