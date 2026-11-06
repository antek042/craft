module;

#include <expected>
#include <string>
#include <vector>

export module craft.util:run_process;
import craft.core;

export namespace craft::util {

struct ProcessResult {
    // Exit code of the program. A program killed by a signal gives
    // 128 + the signal number, like a shell does.
    int exit_code = 0;
    // What the program wrote to stdout and stderr; only set when
    // ProcessOptions::capture_output is true.
    std::string output;
};

struct ProcessOptions {
    // false: the program writes straight to the terminal of the user.
    // true: stdout and stderr are collected into ProcessResult::output.
    bool capture_output = false;
};

// Runs a program and waits for it to finish. `args[0]` is the program (looked
// up in PATH when it has no directory part), the rest are its arguments. No
// shell is involved, so arguments need no quoting. The program inherits the
// environment of Craft.
//
// A program that exits with a non-zero code is not an error: the code is in
// the result. An Error means the program could not be started at all.
[[nodiscard]] std::expected<ProcessResult, Error> RunProcess(
    const std::vector<std::string>& args, const ProcessOptions& options = {});

}  // namespace craft::util
