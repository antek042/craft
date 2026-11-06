module;

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

export module craft.build:generate_cmake;
import craft.core;
import craft.manifest;
import :discover_targets;

export namespace craft::build {

// Writes `text` to `file`, creating the parent directories. Leaves the file
// alone when it already has exactly this content, so CMake doesn't reconfigure
// for nothing. Returns whether the file was written.
[[nodiscard]] std::expected<bool, Error> WriteIfChanged(
    const std::filesystem::path& file, std::string_view text);

enum class Profile : std::uint8_t { kDebug, kRelease };

// Returns the text of the CMakeLists.txt for the project. Pure: no file or
// process access. Paths are taken from `targets` as they are, so they should
// be absolute. The profile sets the default build type.
[[nodiscard]] std::string GenerateCMakeText(const manifest::Manifest& manifest,
                                            const Targets& targets,
                                            Profile profile);

struct BuildOptions {
    std::filesystem::path root;
    Profile profile = Profile::kDebug;
};

class Backend {
public:
    virtual ~Backend() = default;
    // Prepares the project for building. For CMake it writes
    // <root>/target/cmake/CMakeLists.txt.
    virtual std::expected<void, Error> Generate(
        const manifest::Manifest& manifest, const Targets& targets,
        const BuildOptions& options) = 0;
    virtual std::expected<void, Error> Build(const BuildOptions& options) = 0;
};

class CMakeBackend final : public Backend {
public:
    std::expected<void, Error> Generate(const manifest::Manifest& manifest,
                                        const Targets& targets,
                                        const BuildOptions& options) override;
    std::expected<void, Error> Build(const BuildOptions& options) override;
};

}  // namespace craft::build