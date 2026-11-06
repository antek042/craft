module;

#include <filesystem>
#include <ostream>
#include <span>
#include <string_view>

export module craft.cli:build;

import craft.build;

export namespace craft::cli {

// `craft build`: builds the project of the current directory. `args` are the
// arguments after `build`; the only one is `--release`.
//
// Returns the process exit code: 0 on success, 1 on error.
[[nodiscard]] int BuildProject(std::span<const std::string_view> args,
                               std::ostream& out, std::ostream& err);

// Same as above, for the project that contains `dir`, and with the given
// build engine instead of CMake.
[[nodiscard]] int BuildProject(const std::filesystem::path& dir,
                               std::span<const std::string_view> args,
                               std::ostream& out, std::ostream& err,
                               build::Backend& backend);

}  // namespace craft::cli
