module;

#include <string>
#include <string_view>

export module craft.manifest:default_manifest;

export namespace craft::manifest {

// Text of a new craft.toml for a package called `name`. The name must already
// be valid (see ValidatePackageName), so it needs no escaping.
[[nodiscard]] std::string DefaultManifest(std::string_view name);

}  // namespace craft::manifest
