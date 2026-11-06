#ifndef CRAFT_TESTS_UNIT_SCOPED_PATH_H_
#define CRAFT_TESTS_UNIT_SCOPED_PATH_H_

#include <string>

#include "scoped_env.h"

// Sets the PATH environment variable for the lifetime of the object and
// restores it afterwards.
class ScopedPath {
public:
    explicit ScopedPath(const std::string& value) : path_("PATH", value) {}

private:
    ScopedEnv path_;
};

#endif  // CRAFT_TESTS_UNIT_SCOPED_PATH_H_
