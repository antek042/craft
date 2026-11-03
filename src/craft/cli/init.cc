module;

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>

module craft.cli;

import craft.core;
import craft.manifest;
import :create_project;

namespace craft::cli {
namespace {

void PrintSummary(std::ostream& out, std::string_view name,
                  const CreatedProject& project) {
    out << "Created binary package " << name << "\n";
    for (const std::string& file : project.created) {
        out << "  created  " << file << "\n";
    }
    for (const std::string& file : project.skipped) {
        out << "  skipped  " << file << " (already exists)\n";
    }
}

}  // namespace

int InitProject(const std::filesystem::path& dir, std::ostream& out,
                std::ostream& err) {
    const auto name = manifest::PackageNameFromDir(dir);
    if (!name) {
        err << name.error().Format();
        return 1;
    }

    const auto project = CreateProject(dir, *name);
    if (!project) {
        err << project.error().Format();
        return 1;
    }

    PrintSummary(out, *name, *project);
    return 0;
}

int InitProject(std::ostream& out, std::ostream& err) {
    std::error_code ec;
    const std::filesystem::path current_dir = std::filesystem::current_path(ec);
    if (ec) {
        const Error error("could not determine the current directory: " +
                          ec.message());
        err << error.Format();
        return 1;
    }
    return InitProject(current_dir, out, err);
}

}  // namespace craft::cli
