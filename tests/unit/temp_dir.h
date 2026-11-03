#ifndef CRAFT_TESTS_UNIT_TEMP_DIR_H_
#define CRAFT_TESTS_UNIT_TEMP_DIR_H_

#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>

// A fresh temporary directory, removed when the object goes out of scope.
class TempDir {
public:
    TempDir() {
        static int counter = 0;
        // Not file_time_type::clock: its tick type differs between standard
        // libraries (libc++ uses __int128, which std::to_string can't take).
        const auto nanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        path_ = std::filesystem::temp_directory_path() /
                ("craft_test_" + std::to_string(counter++) + "_" +
                 std::to_string(nanoseconds));
        std::filesystem::create_directories(path_);
        // Resolve symlinks (e.g. /tmp on macOS) so paths compare equal.
        path_ = std::filesystem::canonical(path_);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

#endif  // CRAFT_TESTS_UNIT_TEMP_DIR_H_
