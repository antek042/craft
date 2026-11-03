module;

#include <ostream>
#include <span>
#include <string>
#include <string_view>

module craft.cli;

import craft.core;
import :init;
import :read_manifest;

namespace craft::cli {
namespace {

constexpr std::string_view kUsage = R"(A build tool and project manager for C++

Usage: craft [OPTIONS] [COMMAND]

Commands:
    init             Create a new project in the current directory

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
    if (args[0] == "init") {
        if (args.size() > 1) {
            const Error error("`craft init` takes no arguments, got `" +
                                  std::string(args[1]) + "`",
                              "run it inside the directory of the new project");
            err << error.Format();
            return 1;
        }
        return InitProject(out, err);
    }
    if (args[0] == "read-manifest") {
        return ReadManifest(out, err);
    }

    const Error error("unknown command `" + std::string(args[0]) + "`",
                      "run `craft --help` to see available commands");
    err << error.Format();
    return 1;
}

}  // namespace craft::cli
