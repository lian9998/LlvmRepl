#ifndef WRAPPER_GENERATOR_H
#define WRAPPER_GENERATOR_H

#include <string>
#include <memory>
#include <vector>

// Forward declarations to avoid exposing LLVM headers in our API
namespace llvm {
    class Module;
    class LLVMContext;
    class Function;
    class Type;
}

namespace llvm_repl {

/**
 * WrapperGenerator creates i64 wrapper functions for user-defined functions.
 * 
 * Problem: C++ cannot dynamically cast a JIT symbol pointer to an unknown
 * function signature (e.g., "{i32, float} @foo()").
 * 
 * Solution: After parsing a batch but before giving it to the JIT, iterate
 * over the module's functions. For every user function @foo, programmatically
 * generate a wrapper function @__repl_wrap_foo that:
 *   1. Takes no arguments
 *   2. Calls @foo
 *   3. Uses ZExt/PtrToInt to cast whatever @foo returns into a standard i64
 */
class WrapperGenerator {
public:
    /**
     * Result of wrapper generation for a single function.
     */
    struct WrapperInfo {
        std::string originalName;    // e.g., "foo"
        std::string wrapperName;     // e.g., "__repl_wrap_foo"
        bool success;                // Whether wrapper was created successfully
        std::string errorMessage;    // Error message if success is false
    };

    /**
     * Generate wrappers for all user-defined functions in the module.
     * 
     * @param module The LLVM module to process (modified in place)
     * @param context The LLVM context
     * @return Vector of WrapperInfo for each function processed
     */
    static std::vector<WrapperInfo> generateWrappers(
        llvm::Module& module,
        llvm::LLVMContext& context
    );

    /**
     * Get the wrapper function name for a given original function name.
     * @param functionName The original function name (without @)
     * @return The wrapper function name
     */
    static std::string getWrapperName(const std::string& functionName);

    /**
     * Check if a function name is a wrapper function name.
     * @param name The function name to check
     * @return true if this is a wrapper function
     */
    static bool isWrapperName(const std::string& name);

private:
    /**
     * Generate a wrapper for a single function.
     * 
     * @param func The function to wrap
     * @param module The module to add the wrapper to
     * @param context The LLVM context
     * @return WrapperInfo describing the result
     */
    static WrapperInfo generateWrapperForFunction(
        llvm::Function& func,
        llvm::Module& module,
        llvm::LLVMContext& context
    );

    /**
     * Convert a value to i64 using appropriate LLVM instructions.
     * 
     * @param value The value to convert (as a string representation of the type)
     * @param targetType The i64 type
     * @return Instructions to perform the conversion
     */
    static bool canConvertToI64(llvm::Type* type, llvm::LLVMContext& context);
};

} // namespace llvm_repl

#endif // WRAPPER_GENERATOR_H
