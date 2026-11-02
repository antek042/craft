module;

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>

module craft.cli;

import craft.core;
import craft.manifest;

namespace craft::cli {
namespace {

std::string_view ToString(manifest::CppStandard standard) {
    switch (standard) {
        case manifest::CppStandard::k17:
            return "17";
        case manifest::CppStandard::k20:
            return "20";
        case manifest::CppStandard::k23:
            return "23";
        case manifest::CppStandard::k26:
            return "26";
    }
    return "unknown";
}

}  // namespace

int ReadManifest(std::ostream& out, std::ostream& err) {
    std::error_code ec;
    const std::filesystem::path current_dir = std::filesystem::current_path(ec);
    if (ec) {
        const Error error("could not determine the current directory: " +
                          ec.message());
        err << error.Format();
        return 1;
    }

    const auto manifest_path = craft::manifest::FindManifest(current_dir);
    if (!manifest_path) {
        err << manifest_path.error().Format();
        return 1;
    }

    const auto manifest_data = craft::manifest::LoadManifest(*manifest_path);
    if (!manifest_data) {
        err << manifest_data.error().Format();
        return 1;
    }

    const manifest::Package& package = manifest_data->package;
    out << "name = " << package.name << "\n"
        << "version = " << package.version << "\n"
        << "cpp = " << ToString(package.cpp_standard) << "\n";
    return 0;
}

}  // namespace craft::cli
