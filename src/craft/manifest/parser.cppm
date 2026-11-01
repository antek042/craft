module;

#include <expected>
#include <filesystem>
#include <string_view>

export module craft.manifest:parser;

import craft.core;
import :types;

export namespace craft::manifest {

// Parses the text of a `craft.toml` file.
//
// `path` is only used in error messages (`--> path:line:column`); the file
// is not read.
[[nodiscard]] std::expected<Manifest, Error> Parse(
    std::string_view text, const std::filesystem::path& path);

}  // namespace craft::manifest
