#ifndef CRAFT_TESTS_UNIT_SCOPED_ENV_H_
#define CRAFT_TESTS_UNIT_SCOPED_ENV_H_

#include <cstdlib>
#include <optional>
#include <string>

// Sets (or, with std::nullopt, removes) an environment variable for the
// lifetime of the object and restores the old value afterwards.
class ScopedEnv {
public:
    ScopedEnv(std::string name, const std::optional<std::string>& value)
        : name_(std::move(name)) {
        if (const char* old = std::getenv(name_.c_str()); old != nullptr) {
            old_ = old;
        }
        Set(value);
    }
    ~ScopedEnv() { Set(old_); }
    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;

private:
    void Set(const std::optional<std::string>& value) const {
#ifdef _WIN32
        _putenv_s(name_.c_str(), value ? value->c_str() : "");
#else
        if (value) {
            setenv(name_.c_str(), value->c_str(), 1);
        } else {
            unsetenv(name_.c_str());
        }
#endif
    }

    std::string name_;
    std::optional<std::string> old_;
};

#endif  // CRAFT_TESTS_UNIT_SCOPED_ENV_H_
