#include "InputBuffer.h"
#include <algorithm>

namespace llvm_repl {

bool InputBuffer::addLine(const std::string& line) {
    // Check if this line contains the delimiter
    // The delimiter can appear anywhere in the line
    size_t pos = line.find(DELIMITER);
    
    if (pos != std::string::npos) {
        // Found delimiter - add the part before the delimiter
        if (pos > 0) {
            buffer_ << line.substr(0, pos);
        }
        hasCompleteSubmission_ = true;
        return true;
    }
    
    // No delimiter - add the whole line with a newline
    buffer_ << line << '\n';
    return false;
}

bool InputBuffer::hasCompleteSubmission() const {
    return hasCompleteSubmission_;
}

std::string InputBuffer::extractSubmission() {
    std::string result = buffer_.str();
    
    // Clear the buffer
    buffer_.str("");
    buffer_.clear();
    hasCompleteSubmission_ = false;
    
    return result;
}

void InputBuffer::clear() {
    buffer_.str("");
    buffer_.clear();
    hasCompleteSubmission_ = false;
}

bool InputBuffer::empty() const {
    return buffer_.str().empty();
}

std::string InputBuffer::peek() const {
    return buffer_.str();
}

} // namespace llvm_repl
