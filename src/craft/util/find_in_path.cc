module;

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

module craft.util;

namespace craft::util {

namespace {

namespace fs = std::filesystem;

std::string GetEnv(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? value : "";
}

bool IsExecutableFile(const fs::path& file) {
    std::error_code ec;
    if (!fs::is_regular_file(file, ec)) {
        return false;
    }
#ifdef _WIN32
    return true;
#else
    const auto permissions = fs::status(file, ec).permissions();
    return !ec &&
           (permissions & (fs::perms::owner_exec | fs::perms::group_exec |
                           fs::perms::others_exec)) != fs::perms::none;
#endif
}

}  // namespace

std::optional<fs::path> FindInPath(std::string_view name) {
    std::vector<std::string> extensions;
#ifdef _WIN32
    constexpr char kSeparator = ';';
    const std::string pathext = GetEnv("PATHEXT");
    for (const auto part : std::views::split(pathext, kSeparator)) {
        extensions.emplace_back(part.begin(), part.end());
    }
    if (extensions.empty()) {
        extensions = {".exe", ".cmd", ".bat"};
    }
#else
    constexpr char kSeparator = ':';
    extensions = {""};
#endif

    const std::string path = GetEnv("PATH");
    for (const auto part : std::views::split(path, kSeparator)) {
        const std::string_view dir(part.begin(), part.end());
        if (dir.empty()) {
            continue;  // an empty entry means the current directory; skip it
        }
        for (const auto& extension : extensions) {
            fs::path candidate =
                fs::path(dir) / (std::string(name) + extension);
            if (IsExecutableFile(candidate)) {
                return candidate;
            }
        }
    }
    return std::nullopt;
}

}  // namespace craft::util
