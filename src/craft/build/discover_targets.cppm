module;

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module craft.build:discover_targets;
import craft.core;

export namespace craft::build {

// One thing to build: the program or the library.
struct Target {
    // Target name; the package name.
    std::string name;
    // Library only (empty for the program): the package name with '-' replaced
    // by '_'.
    std::string module_name;
    // src/main.<ext> or src/lib.<ext>.
    std::filesystem::path entry;
    // Source files (.cpp .cc .cxx) and module interfaces (.cppm .ixx) that
    // belong to this target, including the entry. Sorted.
    std::vector<std::filesystem::path> sources;
    std::vector<std::filesystem::path> modules;
};

// At least one of the two is set.
struct Targets {
    std::optional<Target> library;
    std::optional<Target> executable;
};

// Finds the targets of the project in `root` by convention. Does not build
// anything. Every file under src/ that is not an entry belongs to the
// library, or to the program if there is no library.
[[nodiscard]] std::expected<Targets, Error> DiscoverTargets(
    const std::filesystem::path& root, std::string_view package_name);

}  // namespace craft::build
