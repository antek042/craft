module;

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

export module craft.cli:create_project;
import craft.core;

export namespace craft::cli {

struct CreatedProject {
    // Files that were written, relative to the project directory,
    // e.g. "craft.toml", "src/main.cpp".
    std::vector<std::string> created;

    // Files that already existed and were left untouched.
    std::vector<std::string> skipped;
};

[[nodiscard]] std::expected<CreatedProject, Error> CreateProject(
    const std::filesystem::path& dir, std::string_view name);

}  // namespace craft::cli
