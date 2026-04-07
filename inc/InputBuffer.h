#ifndef INPUT_BUFFER_H
#define INPUT_BUFFER_H

#include <string>
#include <sstream>

namespace llvm_repl {

/**
 * InputBuffer handles multi-line input batching for the REPL.
 * 
 * Problem: The LLVM IR parser will crash if given an incomplete function body
 * (e.g., just "define i32 @foo() {"). We cannot simply count braces because
 * string literals can contain braces that would break the counting.
 * 
 * Solution: Require the user to terminate submissions with a delimiter ";;".
 * Buffer lines until ";;" is seen, then process the whole string.
 */
class InputBuffer {
public:
    // The delimiter that marks the end of a submission
    static constexpr const char* DELIMITER = ";;";

    InputBuffer() = default;
    ~InputBuffer() = default;

    // Non-copyable, movable
    InputBuffer(const InputBuffer&) = delete;
    InputBuffer& operator=(const InputBuffer&) = delete;
    InputBuffer(InputBuffer&&) = default;
    InputBuffer& operator=(InputBuffer&&) = default;

    /**
     * Add a line of input to the buffer.
     * @param line The line to add
     * @return true if this line completes a submission (contains delimiter)
     */
    bool addLine(const std::string& line);

    /**
     * Check if the buffer contains a complete submission.
     * @return true if a complete submission is ready
     */
    bool hasCompleteSubmission() const;

    /**
     * Extract the complete submission and clear the buffer.
     * @return The complete IR text (without the delimiter)
     */
    std::string extractSubmission();

    /**
     * Clear the buffer without extracting.
     */
    void clear();

    /**
     * Check if the buffer is empty.
     */
    bool empty() const;

    /**
     * Get the current buffered content (for debugging/display).
     */
    std::string peek() const;

private:
    std::stringstream buffer_;
    bool hasCompleteSubmission_ = false;
};

} // namespace llvm_repl

#endif // INPUT_BUFFER_H
