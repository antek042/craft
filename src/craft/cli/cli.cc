module;

#include <ostream>
#include <span>
#include <string>
#include <string_view>

module craft.cli;

import craft.core;

namespace craft::cli {
namespace {

constexpr std::string_view kUsage = R"(A build tool and project manager for C++

Usage: craft [OPTIONS] [COMMAND]

Options:
    -h, --help       Print help
    -V, --version    Print version
)";

}  // namespace

int Run(std::span<const std::string_view> args, std::ostream& out,
        std::ostream& err) {
    if (args.empty() || args[0] == "-h" || args[0] == "--help") {
        out << kUsage;
        return 0;
    }
    if (args[0] == "-V" || args[0] == "--version") {
        out << "craft " << kVersion << "\n";
        return 0;
    }

    const Error error("unknown command `" + std::string(args[0]) + "`",
                      "run `craft --help` to see available commands");
    err << error.Format();
    return 1;
}

}  // namespace craft::cli
