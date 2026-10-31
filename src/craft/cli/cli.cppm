module;

#include <ostream>
#include <span>
#include <string_view>

export module craft.cli;

export namespace craft::cli {

// Runs Craft with the given command-line arguments (without the program
// name). Normal output goes to `out`, errors go to `err`.
//
// Returns the process exit code.
[[nodiscard]] int Run(std::span<const std::string_view> args,
                      std::ostream& out, std::ostream& err);

}  // namespace craft::cli
