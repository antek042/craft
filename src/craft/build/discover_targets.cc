module;

#include <algorithm>
#include <array>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

module craft.build;
import craft.core;
import craft.manifest;

namespace craft::build {

namespace {

namespace fs = std::filesystem;

constexpr std::array<std::string_view, 3> kSourceExtensions = {".cpp", ".cc",
                                                               ".cxx"};
constexpr std::array<std::string_view, 4> kHeaderExtensions = {".h", ".hpp",
                                                               ".hh", ".hxx"};
constexpr std::array<std::string_view, 2> kModuleExtensions = {".cppm", ".ixx"};

template <std::size_t N>
bool HasExtension(const fs::path& file,
                  const std::array<std::string_view, N>& extensions) {
    return std::ranges::find(extensions, file.extension().string()) !=
           extensions.end();
}

bool IsSource(const fs::path& file) {
    return HasExtension(file, kSourceExtensions);
}

bool IsModule(const fs::path& file) {
    return HasExtension(file, kModuleExtensions);
}

// Hidden files and directories (names starting with '.') are ignored.
bool IsHidden(const fs::path& file) {
    const auto name = file.filename().string();
    return !name.empty() && name.front() == '.';
}

// Finds src/<stem>.<ext> directly in `src_dir`. For the library any source,
// module or header extension counts; for the program only a source does.
// Several candidates is an error, because Craft doesn't guess.
std::expected<std::optional<fs::path>, Error> FindEntry(const fs::path& src_dir,
                                                        std::string_view stem,
                                                        bool is_library) {
    std::optional<fs::path> found;

    for (const auto& item : fs::directory_iterator(src_dir)) {
        const auto& file = item.path();
        if (!item.is_regular_file() || file.stem() != stem) {
            continue;
        }
        const bool matches =
            IsSource(file) ||
            (is_library &&
             (IsModule(file) || HasExtension(file, kHeaderExtensions)));
        if (!matches) {
            continue;
        }
        if (found) {
            return std::unexpected(
                Error("more than one src/" + std::string(stem) + ".* file",
                      "keep only one of " + found->filename().string() +
                          " and " + file.filename().string()));
        }
        found = file;
    }
    return found;
}

// All sources and modules under `src_dir`, recursively, sorted.
std::pair<std::vector<fs::path>, std::vector<fs::path>> CollectFiles(
    const fs::path& src_dir) {
    std::vector<fs::path> sources;
    std::vector<fs::path> modules;

    for (auto it = fs::recursive_directory_iterator(src_dir);
         it != fs::recursive_directory_iterator(); ++it) {
        if (IsHidden(it->path())) {
            if (it->is_directory()) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file()) {
            continue;
        }
        if (IsSource(it->path())) {
            sources.push_back(it->path());
        } else if (IsModule(it->path())) {
            modules.push_back(it->path());
        }
    }
    std::ranges::sort(sources);
    std::ranges::sort(modules);
    return {std::move(sources), std::move(modules)};
}

}  // namespace

std::expected<Targets, Error> DiscoverTargets(const fs::path& root,
                                              std::string_view package_name) {
    const fs::path src_dir = root / "src";
    const Error no_targets("no targets found",
                           "create src/main.cc or src/lib.cppm");

    if (!fs::is_directory(src_dir)) {
        return std::unexpected(no_targets);
    }

    auto main_file = FindEntry(src_dir, "main", false);
    if (!main_file) {
        return std::unexpected(main_file.error());
    }
    auto lib_file = FindEntry(src_dir, "lib", true);
    if (!lib_file) {
        return std::unexpected(lib_file.error());
    }
    if (!main_file->has_value() && !lib_file->has_value()) {
        return std::unexpected(no_targets);
    }

    auto [sources, modules] = CollectFiles(src_dir);
    const std::string name(package_name);

    Targets targets;

    if (lib_file->has_value()) {
        Target library;
        library.name = name;
        library.module_name = name;
        std::ranges::replace(library.module_name, '-', '_');
        library.entry = **lib_file;
        library.sources = sources;
        library.modules = modules;
        // The program's entry is not part of the library.
        if (main_file->has_value()) {
            std::erase(library.sources, **main_file);
        }
        targets.library = std::move(library);
    }

    if (main_file->has_value()) {
        Target executable;
        executable.name = name;
        executable.entry = **main_file;
        if (lib_file->has_value()) {
            executable.sources = {**main_file};
        } else {
            executable.sources = sources;
            executable.modules = modules;
        }
        targets.executable = std::move(executable);
    }

    return targets;
}

}  // namespace craft::build
