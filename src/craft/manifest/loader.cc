module;

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

module craft.manifest;

import craft.core;

namespace craft::manifest {
namespace {

constexpr std::string_view kManifestName = "craft.toml";

}  // namespace

std::expected<std::filesystem::path, Error> FindManifest(
    const std::filesystem::path& start) {
    namespace fs = std::filesystem;
    std::error_code ec;

    fs::path dir = fs::absolute(start, ec).lexically_normal();
    if (ec) {
        return std::unexpected(Error("could not resolve path `" +
                                     start.string() + "`: " + ec.message()));
    }

    if (!fs::is_directory(dir, ec)) {
        dir = dir.parent_path();
    }
    // "/a/b/" and "/a/b" are the same directory; keep only one spelling so
    // that the loop below doesn't check it twice.
    if (dir.has_relative_path() && !dir.has_filename()) {
        dir = dir.parent_path();
    }
    const fs::path start_dir = dir;

    while (true) {
        fs::path candidate = dir / kManifestName;
        if (fs::is_regular_file(candidate, ec)) {
            return candidate;
        }

        fs::path parent = dir.parent_path();
        if (parent == dir) {
            break;  // reached the filesystem root
        }
        dir = std::move(parent);
    }

    return std::unexpected(
        Error("could not find `" + std::string(kManifestName) + "` in `" +
                  start_dir.string() + "` or any parent directory",
              "run `craft init` to create a new project here"));
}

}  // namespace craft::manifest
