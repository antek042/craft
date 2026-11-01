module;

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include <toml++/toml.hpp>

module craft.manifest;

import craft.core;

namespace craft::manifest {
namespace {

ErrorLocation LocationOf(const std::string& path,
                         const toml::source_position& position) {
    return ErrorLocation(path, static_cast<unsigned int>(position.line),
                         static_cast<unsigned int>(position.column));
}

// Reads a required string value from [package], e.g. `name = "hello"`.
std::expected<std::string, Error> ReadRequiredString(const toml::table& package,
                                                     std::string_view key,
                                                     const std::string& path) {
    const toml::node* node = package.get(key);
    if (node == nullptr) {
        return std::unexpected(
            Error("missing key `" + std::string(key) + "` in [package]",
                  "add it, e.g. " + std::string(key) + " = \"...\"",
                  LocationOf(path, package.source().begin)));
    }
    if (!node->is_string()) {
        return std::unexpected(Error(
            "`" + std::string(key) + "` must be a string",
            "use a quoted string, e.g. " + std::string(key) + " = \"...\"",
            LocationOf(path, node->source().begin)));
    }
    return node->as_string()->get();
}

}  // namespace

std::expected<Manifest, Error> Parse(std::string_view text,
                                     const std::filesystem::path& path) {
    const std::string path_string = path.string();

    const toml::parse_result result = toml::parse(text, path_string);
    if (!result) {
        const toml::parse_error& error = result.error();
        return std::unexpected(
            Error(std::string(error.description()), {},
                  LocationOf(path_string, error.source().begin)));
    }

    const toml::node* package_node = result.table().get("package");
    if (package_node == nullptr) {
        return std::unexpected(
            Error("missing [package] section",
                  "every craft.toml needs a [package] section"));
    }
    const toml::table* package = package_node->as_table();
    if (package == nullptr) {
        return std::unexpected(
            Error("`package` must be a section", "write it as: [package]",
                  LocationOf(path_string, package_node->source().begin)));
    }

    // TODO: reject unknown keys and sections (separate issue).
    Manifest manifest;

    auto name = ReadRequiredString(*package, "name", path_string);
    if (!name) {
        return std::unexpected(std::move(name.error()));
    }
    manifest.package.name = *std::move(name);

    auto version = ReadRequiredString(*package, "version", path_string);
    if (!version) {
        return std::unexpected(std::move(version.error()));
    }
    manifest.package.version = *std::move(version);

    return manifest;
}

}  // namespace craft::manifest
