module;

#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <ios>
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

std::expected<Manifest, Error> LoadManifest(
    const std::filesystem::path& manifest_path) {
    namespace fs = std::filesystem;
    const std::string path_string = manifest_path.string();
    std::error_code ec;

    // Knowing the size up front lets us detect a short read, which reading
    // through rdbuf() would hide.
    const std::uintmax_t size = fs::file_size(manifest_path, ec);
    if (ec) {
        return std::unexpected(
            Error("could not read `" + path_string + "`: " + ec.message()));
    }

    // Binary mode, so that line endings (and error columns) are the same on
    // every platform.
    std::ifstream file(manifest_path, std::ios::binary);
    if (!file) {
        return std::unexpected(
            Error("could not open `" + path_string + "`",
                  "check that you have permission to read the file"));
    }

    std::string text(size, '\0');
    file.read(text.data(), static_cast<std::streamsize>(size));
    if (file.gcount() != static_cast<std::streamsize>(size)) {
        return std::unexpected(Error("could not read `" + path_string + "`"));
    }

    return Parse(text, manifest_path);
}

}  // namespace craft::manifest
