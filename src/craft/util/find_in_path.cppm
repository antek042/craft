module;

#include <filesystem>
#include <optional>
#include <string_view>

export module craft.util:find_in_path;

export namespace craft::util {

// Finds the program `name` in the directories listed in the PATH environment
// variable and returns its full path. On Windows the extensions from PATHEXT
// are tried too (`cmake` finds `cmake.exe`). Empty PATH entries are skipped.
[[nodiscard]] std::optional<std::filesystem::path> FindInPath(
    std::string_view name);

}  // namespace craft::util
