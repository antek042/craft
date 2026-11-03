module;

#include <string>
#include <string_view>

module craft.manifest;

namespace craft::manifest {

std::string DefaultManifest(std::string_view name) {
    return "[package]\n"
           "name = \"" +
           std::string(name) +
           "\"\n"
           "version = \"0.1.0\"\n"
           "cpp = \"23\"\n";
}

}  // namespace craft::manifest