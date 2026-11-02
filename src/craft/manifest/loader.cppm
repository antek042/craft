module;

#include <expected>
#include <filesystem>

export module craft.manifest:loader;

import craft.core;

export namespace craft::manifest {
[[nodiscard]] std::expected<std::filesystem::path, Error> FindManifest(
    const std::filesystem::path& start);

}  // namespace craft::manifest
