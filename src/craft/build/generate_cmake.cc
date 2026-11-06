module;

#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

module craft.build;
import craft.core;
import craft.manifest;

namespace craft::build {

std::expected<bool, Error> WriteIfChanged(const std::filesystem::path& file,
                                          std::string_view text) {
    namespace fs = std::filesystem;

    std::error_code ec;
    if (fs::is_regular_file(file, ec)) {
        std::ifstream in(file, std::ios::binary);
        const std::string current(std::istreambuf_iterator<char>(in), {});
        if (current == text) {
            return false;
        }
    }

    fs::create_directories(file.parent_path(), ec);
    if (ec) {
        return std::unexpected(Error("cannot create " +
                                     file.parent_path().string() + ": " +
                                     ec.message()));
    }
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!out) {
        return std::unexpected(Error("cannot write " + file.string()));
    }
    return true;
}

std::expected<void, Error> CMakeBackend::Generate(
    const manifest::Manifest& manifest, const Targets& targets,
    const BuildOptions& options) {
    const auto text = GenerateCMakeText(manifest, targets, options.profile);
    const auto file = options.root / "target" / "cmake" / "CMakeLists.txt";

    const auto written = WriteIfChanged(file, text);
    if (!written) {
        return std::unexpected(written.error());
    }
    return {};
}

// TODO(#21): run cmake and ninja.
std::expected<void, Error> CMakeBackend::Build(
    [[maybe_unused]] const BuildOptions& options) {
    return {};
}

}  // namespace craft::build
