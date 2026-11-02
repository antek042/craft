module;

#include <ostream>

export module craft.cli:read_manifest;

export namespace craft::cli {

// Finds and loads the craft.toml of the project in the current directory and
// prints its values to `out`. Errors are printed to `err`.
//
// Returns the process exit code: 0 on success, 1 on error.
[[nodiscard]] int ReadManifest(std::ostream& out, std::ostream& err);

}  // namespace craft::cli