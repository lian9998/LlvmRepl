/**
 * LLVM IR REPL - A Read-Eval-Print Loop for LLVM IR
 * 
 * This REPL behaves like utop for LLVM IR:
 * - Accepts multi-line IR snippets terminated by ";;"
 * - Remembers previously defined functions across submissions
 * - Allows calls to explicit external functions like printf
 * - Automatically evaluates and prints return values
 * 
 * Usage:
 *   ./llvm-repl
 * 
 * Example session:
 *   > define i32 @foo() {
 *   > entry:
 *   >   ret i32 42
 *   > }
 *   > ;;
 *   = 42
 *   > define i32 @bar() {
 *   > entry:
 *   >   %1 = call i32 @foo()
 *   >   %2 = add i32 %1, 8
 *   >   ret i32 %2
 *   > }
 *   > ;;
 *   = 50
 */

#include "REPL.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>

using namespace llvm_repl;

// ANSI color codes for pretty output
namespace Color {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* BOLD = "\033[1m";
}

void printBanner() {
    std::cout << Color::CYAN << Color::BOLD;
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    LLVM IR REPL v1.0                         ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Type LLVM IR code and end submissions with ;;               ║\n";
    std::cout << "║  Commands:                                                   ║\n";
    std::cout << "║    :help     - Show this help message                        ║\n";
    std::cout << "║    :reset    - Clear all definitions and start fresh         ║\n";
    std::cout << "║    :quit     - Exit the REPL                                 ║\n";
    std::cout << "║    :version  - Show version information                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << Color::RESET << std::endl;
}

void printHelp() {
    std::cout << Color::YELLOW << "LLVM IR REPL Help:\n" << Color::RESET;
    std::cout << "  This REPL allows you to interactively write and execute LLVM IR code.\n\n";
    std::cout << "  Key Features:\n";
    std::cout << "    • Multi-line input: Type your IR code across multiple lines,\n";
    std::cout << "      then end with ;; to submit.\n";
    std::cout << "    • State persistence: Functions defined in previous submissions\n";
    std::cout << "      are available in later ones.\n";
    std::cout << "    • External functions: You can declare and use external functions\n";
    std::cout << "      like printf (linked from the host process).\n";
    std::cout << "    • Return values: The REPL automatically prints the return value\n";
    std::cout << "      of the last function defined.\n\n";
    std::cout << "  Example:\n";
    std::cout << "    > define i32 @add(i32 %a, i32 %b) {\n";
    std::cout << "    > entry:\n";
    std::cout << "    >   %result = add i32 %a, %b\n";
    std::cout << "    >   ret i32 %result\n";
    std::cout << "    > }\n";
    std::cout << "    > ;;\n\n";
    std::cout << "  Commands:\n";
    std::cout << "    :help     - Show this help message\n";
    std::cout << "    :reset    - Clear all definitions and start fresh\n";
    std::cout << "    :quit     - Exit the REPL\n";
    std::cout << "    :version  - Show version information\n";
    std::cout << std::endl;
}

void printVersion() {
    std::cout << "not impl." << std::endl;
}

void printPrompt(int lineNum) {
    if (lineNum == 0) {
        std::cout << Color::GREEN << "> " << Color::RESET;
    } else {
        std::cout << Color::BLUE << "| " << Color::RESET;
    }
    std::cout.flush();
}

void printResult(const EvaluationResult& result) {
    if (!result.success) {
        std::cout << Color::RED << "Error: " << result.errorMessage << Color::RESET << "\n";
        return;
    }
    
    if (!result.lastFunctionName.empty()) {
        std::cout << Color::GREEN << "= " << Color::RESET << result.returnValue;
        std::cout << Color::YELLOW << " ; " << Color::RESET;
        std::cout << "(" << result.lastFunctionName << ")\n";
    }
    
    // Print wrapper information for debugging
    if (!result.wrappers.empty()) {
        std::cout << Color::CYAN << "  Defined functions: " << Color::RESET;
        bool first = true;
        for (const auto& info : result.wrappers) {
            if (info.success) {
                if (!first) std::cout << ", ";
                std::cout << "@" << info.originalName;
                first = false;
            }
        }
        std::cout << "\n";
    }
}

bool handleCommand(const std::string& line, REPL& repl, bool& shouldQuit) {
    std::string trimmed = line;
    
    // Trim leading whitespace
    size_t start = trimmed.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return false;
    }
    trimmed = trimmed.substr(start);
    
    // Check if it's a command
    if (trimmed[0] != ':') {
        return false;
    }
    
    // Extract command
    std::string command = trimmed.substr(1);
    
    // Trim trailing whitespace
    size_t end = command.find_last_not_of(" \t\n\r");
    if (end != std::string::npos) {
        command = command.substr(0, end + 1);
    }
    
    if (command == "quit" || command == "q" || command == "exit") {
        shouldQuit = true;
        return true;
    }
    else if (command == "help" || command == "h" || command == "?") {
        printHelp();
        return true;
    }
    else if (command == "reset" || command == "r") {
        repl.reset();
        std::cout << Color::YELLOW << "REPL reset. All previous definitions cleared.\n" << Color::RESET;
        return true;
    }
    else if (command == "version" || command == "v") {
        printVersion();
        return true;
    }
    else {
        std::cout << Color::RED << "Unknown command: :" << command << "\n" << Color::RESET;
        std::cout << "Type :help for available commands.\n";
        return true;
    }
}

int main(int argc, char* argv[]) {
    // Check for --help or --version flags
    bool stealth = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -h, --help     Show this help message\n";
            std::cout << "  -v, --version  Show version information\n";
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printVersion();
            return 0;
        }
        if (strcmp(argv[i], "--stealth") == 0 || strcmp(argv[i], "-a") == 0) {
            stealth = true;
        }
    }
    if(!stealth) {
        printBanner();
    }
    
    // Create the REPL
    REPL repl;
    
    if (!repl.isValid()) {
        std::cerr << Color::RED << "Failed to initialize REPL. Exiting.\n" << Color::RESET;
        return 1;
    }
    
    // Set up output callback for printf
    repl.setOutputCallback([](const std::string& output) {
        std::cout << output;
    });
    
    std::string line;
    int lineNum = 0;
    bool shouldQuit = false;
    
    while (!shouldQuit) {
    if(!stealth) {
        printPrompt(lineNum);
    }        
        if (!std::getline(std::cin, line)) {
            // EOF reached
            std::cout << "\n";
            break;
        }
        
        // Check for commands
        if (handleCommand(line, repl, shouldQuit)) {
            lineNum = 0;
            continue;
        }
        
        // Process the line
        auto result = repl.processLine(line);
        
        if (result.has_value() && !stealth) {
            printResult(result.value());
            lineNum = 0;
        } else {
            lineNum++;
        }
    }

    if(!stealth) {
        std::cout << Color::GREEN << "Goodbye!\n" << Color::RESET;
    }    
    return 0;
}
