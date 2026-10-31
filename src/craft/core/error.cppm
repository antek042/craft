module;

#include <optional>
#include <string>
#include <utility>

export module craft.core:error;

export namespace craft {

struct ErrorLocation {
    std::string file_path;
    unsigned int line = 0;
    unsigned int column = 0;

    ErrorLocation() = default;

    ErrorLocation(std::string file_path, unsigned int line, unsigned int column)
        : file_path(std::move(file_path)), line(line), column(column) {}
};

// An error that is shown to the user, e.g. a missing file or a bad manifest.
//
// Functions that can fail return std::expected<T, Error>. The message says
// what went wrong; the optional hint says how to fix it.
class Error {
public:
    explicit Error(std::string message, std::string hint = {},
                   std::optional<ErrorLocation> error_location = std::nullopt);

    [[nodiscard]] const std::string& message() const { return message_; }
    [[nodiscard]] const std::string& hint() const { return hint_; }

    // Returns the error as printed to the terminal:
    //
    //   error: <message>
    //     --> <file>:<line>:<column>
    //   hint: <hint>
    //
    // The location line and the hint line are left out when not set.
    [[nodiscard]] std::string Format() const;

private:
    std::string message_;
    std::string hint_;
    std::optional<ErrorLocation> error_location_;
};

}  // namespace craft
