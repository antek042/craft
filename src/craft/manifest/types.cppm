module;

#include <cstdint>
#include <string>

export module craft.manifest:types;

export namespace craft::manifest {

enum class CppStandard : std::uint8_t { k17, k20, k23, k26 };

struct Package {
    std::string name;
    std::string version;
    CppStandard cpp_standard = CppStandard::k23;
    std::string description;
    std::string license;
};

struct Manifest {
    Package package;
};

}  // namespace craft::manifest
