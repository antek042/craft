module;

#include <filesystem>
#include <ostream>

export module craft.cli:init;

export namespace craft::cli {

// `craft init`: creates a new project in the current directory. The package
// name is taken from the name of the directory.
//
// Returns the process exit code.
[[nodiscard]] int InitProject(std::ostream& out, std::ostream& err);

// Same as above, in `dir` instead of the current directory.
[[nodiscard]] int InitProject(const std::filesystem::path& dir,
                              std::ostream& out, std::ostream& err);

}  // namespace craft::cli
