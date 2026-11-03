module;

#include <algorithm>
#include <cstddef>
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

constexpr std::size_t kMaxPackageNameLength = 64;

// Plain ASCII checks: the rules must not depend on the current locale.
bool IsLower(char c) { return c >= 'a' && c <= 'z'; }
bool IsUpper(char c) { return c >= 'A' && c <= 'Z'; }
bool IsDigit(char c) { return c >= '0' && c <= '9'; }

char ToLower(char c) {
    return IsUpper(c) ? static_cast<char>(c - 'A' + 'a') : c;
}

}  // namespace

std::expected<void, Error> ValidatePackageName(std::string_view name) {
    const std::string quoted = "`" + std::string(name) + "`";

    if (name.empty()) {
        return std::unexpected(
            Error("package name is empty", "use a name like \"hello\""));
    }

    if (name.size() > kMaxPackageNameLength) {
        return std::unexpected(
            Error("package name " + quoted + " is too long (" +
                      std::to_string(name.size()) + " characters, at most " +
                      std::to_string(kMaxPackageNameLength) + ")",
                  "use a shorter name"));
    }

    for (const char c : name) {
        if (!IsLower(c) && !IsUpper(c) && !IsDigit(c) && c != '-' && c != '_') {
            return std::unexpected(Error(
                "package name " + quoted + " contains an invalid character '" +
                    std::string(1, c) + "'",
                "use only lowercase letters, digits, '-' and '_'"));
        }
    }

    if (std::ranges::any_of(name, IsUpper)) {
        std::string lowercase(name);
        std::ranges::transform(lowercase, lowercase.begin(), ToLower);
        return std::unexpected(
            Error("package name " + quoted + " must be lowercase",
                  "use lowercase, e.g. \"" + lowercase + "\""));
    }

    if (!IsLower(name.front())) {
        return std::unexpected(
            Error("package name " + quoted + " must start with a letter",
                  "start the name with a letter, e.g. \"hello\""));
    }

    return {};
}

std::expected<std::string, Error> PackageNameFromDir(
    const std::filesystem::path& dir) {
    namespace fs = std::filesystem;
    std::error_code ec;

    fs::path normalised = fs::absolute(dir, ec).lexically_normal();
    if (ec) {
        return std::unexpected(Error("could not resolve path `" + dir.string() +
                                     "`: " + ec.message()));
    }

    // "dir/" and "dir/." end with a separator and have an empty filename().
    if (!normalised.has_filename()) {
        normalised = normalised.parent_path();
    }

    const std::string directory_name = normalised.filename().string();
    std::string name = directory_name;
    std::ranges::transform(name, name.begin(), ToLower);

    if (const auto valid = ValidatePackageName(name); !valid) {
        return std::unexpected(
            Error("cannot derive a package name from directory `" +
                      directory_name + "`: " + valid.error().message(),
                  "rename the directory, or write craft.toml by hand"));
    }
    return name;
}

}  // namespace craft::manifest
