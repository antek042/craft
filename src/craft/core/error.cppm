module;

#include <string>

export module craft.core:error;

export namespace craft {

// An error that is shown to the user, e.g. a missing file or a bad manifest.
//
// Functions that can fail return std::expected<T, Error>. The message says
// what went wrong; the optional hint says how to fix it.
class Error {
public:
    explicit Error(std::string message, std::string hint = {});

    const std::string& message() const { return message_; }
    const std::string& hint() const { return hint_; }

    // Returns the error as printed to the terminal:
    //
    //   error: <message>
    //   hint: <hint>
    [[nodiscard]] std::string Format() const;

private:
    std::string message_;
    std::string hint_;
};

}  // namespace craft
