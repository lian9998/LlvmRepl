#ifndef REPL_H
#define REPL_H

#include "InputBuffer.h"
#include "WrapperGenerator.h"
#include <memory>
#include <string>
#include <functional>
#include <optional>

// Forward declarations
namespace llvm {
    class LLVMContext;
    class Module;
    class Error;
    class StringRef;
    namespace orc {
        class LLJIT;
        class ThreadSafeModule;
    }
}

namespace llvm_repl {

/**
 * Result of evaluating an IR submission.
 */
struct EvaluationResult {
    bool success;
    std::string lastFunctionName;   // Name of the last function defined
    int64_t returnValue;            // Return value from the wrapper
    std::string output;             // Output from printf, etc.
    std::string errorMessage;       // Error message if success is false
    std::vector<WrapperGenerator::WrapperInfo> wrappers; // Generated wrappers
};

/**
 * The main REPL class that orchestrates the LLVM IR read-eval-print loop.
 * 
 * Key features:
 * 1. Text Batching: Buffers input until ";;" delimiter
 * 2. JIT Batching: Maintains state across submissions using LLJIT
 * 3. External Linking: Allows calls to printf and other libc functions
 * 4. Return Value Wrapping: Automatically generates i64 wrappers
 */
class REPL {
public:
    /**
     * Output callback type for printf and other output functions.
     */
    using OutputCallback = std::function<void(const std::string&)>;

    /**
     * Construct a new REPL instance.
     * Initializes the JIT and sets up external symbol linking.
     */
    REPL();

    /**
     * Destructor.
     */
    ~REPL();

    // Non-copyable, non-movable (owns unique resources)
    REPL(const REPL&) = delete;
    REPL& operator=(const REPL&) = delete;
    REPL(REPL&&) = delete;
    REPL& operator=(REPL&&) = delete;

    /**
     * Process a line of input.
     * 
     * @param line The input line
     * @return Optional EvaluationResult if this line completed a submission
     */
    std::optional<EvaluationResult> processLine(const std::string& line);

    /**
     * Process a complete IR submission directly.
     * 
     * @param irCode The complete IR code to process
     * @return EvaluationResult containing the result
     */
    EvaluationResult processSubmission(const std::string& irCode);

    /**
     * Set the output callback for printf output.
     * @param callback The callback function
     */
    void setOutputCallback(OutputCallback callback);

    /**
     * Get the input buffer (for querying state).
     */
    const InputBuffer& getInputBuffer() const { return inputBuffer_; }

    /**
     * Clear all state and reset the REPL.
     * Note: This creates a new JIT instance, clearing all previous definitions.
     */
    void reset();

    /**
     * Check if the REPL is in a valid state.
     */
    bool isValid() const { return jit_ != nullptr; }

private:
    /**
     * Initialize the LLJIT instance with external symbol linking.
     */
    bool initializeJIT();

    /**
     * Parse IR code into a module.
     * 
     * @param irCode The IR code to parse
     * @param context The LLVM context (created fresh for each batch)
     * @return Unique pointer to the module, or nullptr on error
     */
    std::unique_ptr<llvm::Module> parseIR(
        const std::string& irCode,
        std::unique_ptr<llvm::LLVMContext>& context
    );

    /**
     * Add a module to the JIT.
     * 
     * @param module The module to add
     * @param context The context (ownership transferred to ThreadSafeModule)
     * @return true on success
     */
    bool addModuleToJIT(
        std::unique_ptr<llvm::Module> module,
        std::unique_ptr<llvm::LLVMContext> context
    );

    /**
     * Look up a symbol in the JIT.
     * 
     * @param name The symbol name
     * @return The symbol address, or 0 on error
     */
    uint64_t lookupSymbol(const std::string& name);

    /**
     * Find the name of the last function defined in a module.
     * 
     * @param module The module to search
     * @return The name of the last defined function, or empty string
     */
    std::string findLastFunctionName(llvm::Module& module);

    /**
     * Execute a wrapper function and return the result.
     * 
     * @param wrapperName The name of the wrapper function
     * @return The return value
     */
    int64_t executeWrapper(const std::string& wrapperName);

private:
    InputBuffer inputBuffer_;
    std::unique_ptr<llvm::orc::LLJIT> jit_;
    OutputCallback outputCallback_;
    int batchNumber_ = 0;
};

} // namespace llvm_repl

#endif // REPL_H
