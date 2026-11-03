module;

#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <ios>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

module craft.cli;

import craft.core;
import craft.manifest;

namespace craft::cli {

namespace {

enum class FileStatus : std::uint8_t { kCreated, kSkipped };

// Writes `text` to `path` unless the file already exists. Does not create
// directories.
std::expected<FileStatus, Error> WriteFileIfMissing(
    const std::filesystem::path& path, std::string_view text) {
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        return FileStatus::kSkipped;
    }
    if (ec) {
        return std::unexpected(
            Error("could not access `" + path.string() + "`: " + ec.message()));
    }

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected(
            Error("could not create `" + path.string() + "`",
                  "check that you have permission to write there"));
    }

    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    file.close();
    if (!file) {
        // Don't leave a half-written file behind.
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        return std::unexpected(
            Error("could not write `" + path.string() + "`"));
    }
    return FileStatus::kCreated;
}

std::string_view DefaultSourceFile() {
    // The delimiter is not `cpp`: clang-format would reformat that as code.
    return R"src(#include <iostream>

int main() {
    std::cout << "Hello, world!\n";
}
)src";
}

std::string_view DefaultGitignore() { return "/target\n"; }

}  // namespace

std::expected<CreatedProject, Error> CreateProject(
    const std::filesystem::path& dir, std::string_view name) {
    namespace fs = std::filesystem;
    std::error_code ec;

    const bool dir_exists = fs::exists(dir, ec);
    if (ec) {
        return std::unexpected(
            Error("could not access `" + dir.string() + "`: " + ec.message()));
    }
    if (!dir_exists) {
        return std::unexpected(
            Error("directory `" + dir.string() + "` does not exist"));
    }
    const bool is_directory = fs::is_directory(dir, ec);
    if (ec) {
        return std::unexpected(
            Error("could not access `" + dir.string() + "`: " + ec.message()));
    }
    if (!is_directory) {
        return std::unexpected(
            Error("`" + dir.string() + "` is not a directory"));
    }

    if (const auto valid = manifest::ValidatePackageName(name); !valid) {
        return std::unexpected(valid.error());
    }

    const std::string manifest_name(manifest::kManifestName);
    const bool has_manifest = fs::exists(dir / manifest_name, ec);
    if (ec) {
        return std::unexpected(Error("could not access `" +
                                     (dir / manifest_name).string() +
                                     "`: " + ec.message()));
    }
    if (has_manifest) {
        return std::unexpected(Error(
            "`" + manifest_name + "` already exists in `" + dir.string() + "`",
            "edit it, or run this in an empty directory"));
    }

    const fs::path src_dir = dir / "src";
    const bool created_src_dir = fs::create_directory(src_dir, ec);
    if (ec) {
        return std::unexpected(Error("could not create `" + src_dir.string() +
                                     "`: " + ec.message()));
    }

    CreatedProject project;
    std::vector<fs::path> written;  // files created by this call

    const auto fail = [&](Error error) {
        std::error_code ignored;
        for (const fs::path& file : std::views::reverse(written)) {
            fs::remove(file, ignored);
        }
        if (created_src_dir) {
            fs::remove(src_dir, ignored);
        }
        return std::unexpected(std::move(error));
    };

    const auto write =
        [&](const fs::path& relative,
            std::string_view text) -> std::expected<FileStatus, Error> {
        const auto status = WriteFileIfMissing(dir / relative, text);
        if (status) {
            if (*status == FileStatus::kCreated) {
                written.push_back(dir / relative);
                project.created.push_back(relative.generic_string());
            } else {
                project.skipped.push_back(relative.generic_string());
            }
        }
        return status;
    };

    const auto manifest_status =
        write(manifest_name, manifest::DefaultManifest(name));
    if (!manifest_status) {
        return fail(manifest_status.error());
    }
    if (*manifest_status == FileStatus::kSkipped) {
        return fail(Error("`" + manifest_name + "` already exists in `" +
                          dir.string() + "`"));
    }

    if (const auto status =
            write(fs::path("src") / "main.cpp", DefaultSourceFile());
        !status) {
        return fail(status.error());
    }

    if (const auto status = write(".gitignore", DefaultGitignore()); !status) {
        return fail(status.error());
    }

    return project;
}

}  // namespace craft::cli
