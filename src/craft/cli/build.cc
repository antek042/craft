module;

#include <expected>
#include <filesystem>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

module craft.cli;

import craft.build;
import craft.core;
import craft.manifest;

namespace craft::cli {
namespace {

struct BuildArguments {
    build::Profile profile = build::Profile::kDebug;
};

std::expected<BuildArguments, Error> ParseArguments(
    std::span<const std::string_view> args) {
    BuildArguments parsed;
    for (const std::string_view arg : args) {
        if (arg == "--release") {
            parsed.profile = build::Profile::kRelease;
        } else if (arg.starts_with('-')) {
            return std::unexpected(Error(
                "unknown flag `" + std::string(arg) + "` for `craft build`",
                "the only flag is `--release`"));
        } else {
            return std::unexpected(
                Error("`craft build` takes no arguments, got `" +
                          std::string(arg) + "`",
                      "run it inside the directory of the project"));
        }
    }
    return parsed;
}

// A status line in the style of Cargo: the word is right-aligned.
//
//    Compiling hello v0.1.0 (/home/me/hello)
//     Finished debug target
std::string Status(std::string_view word, std::string_view text) {
    constexpr std::size_t kWidth = 12;
    std::string line(word.size() < kWidth ? kWidth - word.size() : 0, ' ');
    line += word;
    line += ' ';
    line += text;
    return line;
}

}  // namespace

int BuildProject(const std::filesystem::path& dir,
                 std::span<const std::string_view> args, std::ostream& out,
                 std::ostream& err, build::Backend& backend) {
    const auto arguments = ParseArguments(args);
    if (!arguments) {
        err << arguments.error().Format();
        return 1;
    }

    const auto manifest_path = manifest::FindManifest(dir);
    if (!manifest_path) {
        err << manifest_path.error().Format();
        return 1;
    }
    const auto manifest = manifest::LoadManifest(*manifest_path);
    if (!manifest) {
        err << manifest.error().Format();
        return 1;
    }

    const std::filesystem::path root = manifest_path->parent_path();
    const auto targets = build::DiscoverTargets(root, manifest->package.name);
    if (!targets) {
        err << targets.error().Format();
        return 1;
    }

    const build::BuildOptions options{.root = root,
                                      .profile = arguments->profile};
    const auto generated = backend.Generate(*manifest, *targets, options);
    if (!generated) {
        err << generated.error().Format();
        return 1;
    }

    out << Status("Compiling", manifest->package.name + " v" +
                                   manifest->package.version + " (" +
                                   root.string() + ")")
        << "\n";
    const auto built = backend.Build(options);
    if (!built) {
        err << built.error().Format();
        return 1;
    }

    const bool release = arguments->profile == build::Profile::kRelease;
    out << Status("Finished", release ? "release target" : "debug target")
        << "\n";
    return 0;
}

int BuildProject(std::span<const std::string_view> args, std::ostream& out,
                 std::ostream& err) {
    std::error_code ec;
    const std::filesystem::path current_dir = std::filesystem::current_path(ec);
    if (ec) {
        const Error error("could not determine the current directory: " +
                          ec.message());
        err << error.Format();
        return 1;
    }
    build::CMakeBackend backend;
    return BuildProject(current_dir, args, out, err, backend);
}

}  // namespace craft::cli
