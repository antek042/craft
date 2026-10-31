module;

#include <optional>
#include <string>
#include <utility>

module craft.core;

namespace craft {

Error::Error(std::string message, std::string hint,
             std::optional<ErrorLocation> error_location)
    : message_(std::move(message)),
      hint_(std::move(hint)),
      error_location_(std::move(error_location)) {}

std::string Error::Format() const {
    std::string text = "error: " + message_ + "\n";
    if (error_location_) {
        text += "  --> " + error_location_->file_path + ":" +
                std::to_string(error_location_->line) + ":" +
                std::to_string(error_location_->column) + "\n";
    }
    if (!hint_.empty()) {
        text += "hint: " + hint_ + "\n";
    }
    return text;
}

}  // namespace craft
