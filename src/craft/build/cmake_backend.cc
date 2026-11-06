module;

#include <cstdlib>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

module craft.build;
import craft.core;
import craft.manifest;
import craft.util;

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

namespace {

std::string ProfileDirectory(Profile profile) {
    return profile == Profile::kRelease ? "release" : "debug";
}

std::string BuildTypeName(Profile profile) {
    return profile == Profile::kRelease ? "Release" : "Debug";
}

// Runs one step. The output of the program already went to the user, so a
// failure only needs to name the step.
std::expected<void, Error> RunStep(const std::string& step,
                                   const std::vector<std::string>& args) {
    const auto result = util::RunProcess(args);
    if (!result) {
        return std::unexpected(result.error());
    }
    if (result->exit_code != 0) {
        return std::unexpected(Error(step + " failed (exit code " +
                                     std::to_string(result->exit_code) + ")"));
    }
    return {};
}

}  // namespace

std::expected<void, Error> CMakeBackend::Build(const BuildOptions& options) {
    const auto cmake = util::FindInPath("cmake");
    if (!cmake) {
        return std::unexpected(
            Error("cmake not found in PATH",
                  "craft needs cmake 3.28 or newer to build projects"));
    }
    const auto ninja = util::FindInPath("ninja");
    if (!ninja) {
        return std::unexpected(
            Error("ninja not found in PATH",
                  "craft needs ninja to build projects; install it and add "
                  "it to PATH"));
    }

    const auto source_dir = options.root / "target" / "cmake";
    const auto build_dir =
        options.root / "target" / ProfileDirectory(options.profile);

    std::vector<std::string> configure = {
        cmake->string(),
        "-S",
        source_dir.string(),
        "-B",
        build_dir.string(),
        "-G",
        "Ninja",
        "-DCMAKE_BUILD_TYPE=" + BuildTypeName(options.profile),
        "-DCMAKE_MAKE_PROGRAM=" + ninja->string()};
    // The CXX environment variable picks the compiler. CMake reads it only
    // when it creates the build tree and then remembers the compiler, so pass
    // it every time: CMake starts over when it changes.
    if (const char* cxx = std::getenv("CXX"); cxx != nullptr && *cxx != '\0') {
        configure.push_back(std::string("-DCMAKE_CXX_COMPILER=") + cxx);
    }

    auto step = RunStep("cmake configure", configure);
    if (!step) {
        return step;
    }
    return RunStep("cmake build",
                   {cmake->string(), "--build", build_dir.string()});
}

}  // namespace craft::build
