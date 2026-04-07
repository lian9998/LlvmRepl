#include "REPL.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/ExecutionEngine/Orc/SymbolStringPool.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <iostream>
#include <sstream>

namespace llvm_repl {

// Static initialization for LLVM targets
static struct LLVMInitializer {
    LLVMInitializer() {
        // Initialize all targets for JIT compilation
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
    }
} llvmInitializer;

REPL::REPL() : inputBuffer_(), jit_(nullptr), outputCallback_(nullptr), batchNumber_(0) {
    initializeJIT();
}

REPL::~REPL() {
    jit_.reset();
}

bool REPL::initializeJIT() {
    using namespace llvm;
    using namespace llvm::orc;
    
    // Create the LLJIT instance
    auto jitBuilder = LLJITBuilder();
    
    Expected<std::unique_ptr<LLJIT>> jitOrErr = jitBuilder.create();
    if (!jitOrErr) {
        std::cerr << "Error creating LLJIT: " << toString(jitOrErr.takeError()) << std::endl;
        return false;
    }
    
    jit_ = std::move(*jitOrErr);
    
    // Add a DynamicLibrarySearchGenerator for the current process
    // This allows the JIT to find symbols like printf from the host process
    auto dlsymGenerator = DynamicLibrarySearchGenerator::GetForCurrentProcess(
        jit_->getDataLayout().getGlobalPrefix()
    );
    
    if (!dlsymGenerator) {
        std::cerr << "Error creating process symbol generator: " 
                  << toString(dlsymGenerator.takeError()) << std::endl;
        return false;
    }
    
    // Add the generator to the main JITDylib
    jit_->getMainJITDylib().addGenerator(std::move(*dlsymGenerator));
    
    return true;
}

std::optional<EvaluationResult> REPL::processLine(const std::string& line) {
    bool completed = inputBuffer_.addLine(line);
    
    if (completed && inputBuffer_.hasCompleteSubmission()) {
        std::string submission = inputBuffer_.extractSubmission();
        return processSubmission(submission);
    }
    
    return std::nullopt;
}

EvaluationResult REPL::processSubmission(const std::string& irCode) {
    EvaluationResult result;
    result.success = false;
    result.returnValue = 0;
    
    // Skip empty submissions
    if (irCode.empty() || irCode.find_first_not_of(" \t\n\r") == std::string::npos) {
        result.success = true;
        return result;
    }
    
    // Create a fresh context for this batch
    // This is important: if parsing fails, we just throw away this context
    // The JIT and previous valid code remain healthy
    auto context = std::make_unique<llvm::LLVMContext>();
    
    // Parse the IR
    std::unique_ptr<llvm::Module> module = parseIR(irCode, context);
    if (!module) {
        result.errorMessage = "Failed to parse IR";
        return result;
    }
    
    // Find the last function name before adding wrappers
    result.lastFunctionName = findLastFunctionName(*module);
    
    // Generate wrappers for all user-defined functions
    result.wrappers = WrapperGenerator::generateWrappers(*module, *context);
    
    // Add the module to the JIT
    if (!addModuleToJIT(std::move(module), std::move(context))) {
        result.errorMessage = "Failed to add module to JIT";
        return result;
    }
    
    // If there was a last function, execute its wrapper
    if (!result.lastFunctionName.empty()) {
        std::string wrapperName = WrapperGenerator::getWrapperName(result.lastFunctionName);
        
        // Check if the wrapper was generated successfully
        bool wrapperExists = false;
        for (const auto& info : result.wrappers) {
            if (info.originalName == result.lastFunctionName && info.success) {
                wrapperExists = true;
                break;
            }
        }
        
        if (wrapperExists) {
            result.returnValue = executeWrapper(wrapperName);
        }
    }
    
    result.success = true;
    batchNumber_++;
    
    return result;
}

void REPL::setOutputCallback(OutputCallback callback) {
    outputCallback_ = std::move(callback);
}

void REPL::reset() {
    jit_.reset();
    inputBuffer_.clear();
    batchNumber_ = 0;
    initializeJIT();
}

std::unique_ptr<llvm::Module> REPL::parseIR(
    const std::string& irCode,
    std::unique_ptr<llvm::LLVMContext>& context
) {
    using namespace llvm;
    
    SMDiagnostic err;
    std::unique_ptr<Module> module_ = llvm::parseIR(
        MemoryBufferRef(irCode, "<repl-input>"),
        err,
        *context
    );
    
    if (!module_) {
        std::string errMsg;
        raw_string_ostream rso(errMsg);
        err.print("<repl-input>", rso);
        std::cerr << "Parse error: " << rso.str() << std::endl;
        return nullptr;
    }
    
    return module_;
}

bool REPL::addModuleToJIT(
    std::unique_ptr<llvm::Module> module,
    std::unique_ptr<llvm::LLVMContext> context
) {
    using namespace llvm::orc;
    
    // Create a ThreadSafeModule
    ThreadSafeModule tsm(std::move(module), std::move(context));
    
    // Add to the JIT
    llvm::Error err = jit_->addIRModule(std::move(tsm));
    
    if (err) {
        std::cerr << "Error adding module: " << llvm::toString(std::move(err)) << std::endl;
        return false;
    }
    
    return true;
}

uint64_t REPL::lookupSymbol(const std::string& name) {
    using namespace llvm::orc;
    
    auto symbol = jit_->lookup(name);
    
    if (!symbol) {
        llvm::errs() << "Symbol not found: " << name << "\n";
        llvm::consumeError(symbol.takeError());
        return 0;
    }
    
    return symbol->getValue();
}

std::string REPL::findLastFunctionName(llvm::Module& module) {
    std::string lastName;
    
    for (llvm::Function& func : module.functions()) {
        // Skip declarations
        if (func.isDeclaration()) {
            continue;
        }
        
        // Skip wrapper functions
        if (WrapperGenerator::isWrapperName(func.getName().str())) {
            continue;
        }
        
        // Skip internal functions
        if (func.getName().starts_with(".")) {
            continue;
        }
        
        lastName = func.getName().str();
    }
    
    return lastName;
}

int64_t REPL::executeWrapper(const std::string& wrapperName) {
    uint64_t addr = lookupSymbol(wrapperName);
    
    if (addr == 0) {
        return 0;
    }
    
    // Cast to function pointer and call
    auto* fnPtr = reinterpret_cast<int64_t (*)()>(addr);
    
    return fnPtr();
}

} // namespace llvm_repl
