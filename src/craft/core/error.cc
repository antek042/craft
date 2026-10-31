module;

#include <string>
#include <utility>

module craft.core;

namespace craft {

Error::Error(std::string message, std::string hint)
    : message_(std::move(message)), hint_(std::move(hint)) {}

std::string Error::Format() const {
    std::string text = "error: " + message_ + "\n";
    if (!hint_.empty()) {
        text += "hint: " + hint_ + "\n";
    }
    return text;
}

}  // namespace craft
