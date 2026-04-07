#include "WrapperGenerator.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>

namespace llvm_repl {

std::string WrapperGenerator::getWrapperName(const std::string& functionName) {
    return "__repl_wrap_" + functionName;
}

bool WrapperGenerator::isWrapperName(const std::string& name) {
    return name.find("__repl_wrap_") == 0;
}

bool WrapperGenerator::canConvertToI64(llvm::Type* type, llvm::LLVMContext& context) {
    if (!type) return false;
    
    // We can convert any integer type to i64
    if (type->isIntegerTy()) {
        return true;
    }
    
    // We can convert pointer types to i64
    if (type->isPointerTy()) {
        return true;
    }
    
    // We can convert float/double to i64 (via bitcast or conversion)
    if (type->isFloatTy() || type->isDoubleTy()) {
        return true;
    }
    
    // We can convert void (returns 0)
    if (type->isVoidTy()) {
        return true;
    }
    
    // For structs and arrays, we need to check if they fit in i64
    // For simplicity, we'll try to handle small structs
    if (type->isStructTy() || type->isArrayTy()) {
        // Check if the size fits in i64
        llvm::DataLayout dl(""); // Default data layout
        uint64_t size = dl.getTypeAllocSize(type);
        return size <= 8; // i64 is 8 bytes
    }
    
    return false;
}

std::vector<WrapperGenerator::WrapperInfo> WrapperGenerator::generateWrappers(
    llvm::Module& module,
    llvm::LLVMContext& context
) {
    std::vector<WrapperInfo> results;
    
    // Collect all functions first (to avoid iterator invalidation)
    std::vector<llvm::Function*> functions;
    for (llvm::Function& func : module.functions()) {
        functions.push_back(&func);
    }
    
    for (llvm::Function* func : functions) {
        // Skip declarations (external functions like printf)
        if (func->isDeclaration()) {
            continue;
        }
        
        // Skip functions that are already wrappers
        if (isWrapperName(func->getName().str())) {
            continue;
        }
        
        // Skip internal functions (starting with .)
        if (func->getName().starts_with(".")) {
            continue;
        }
        
        WrapperInfo info = generateWrapperForFunction(*func, module, context);
        results.push_back(info);
    }
    
    return results;
}

WrapperGenerator::WrapperInfo WrapperGenerator::generateWrapperForFunction(
    llvm::Function& func,
    llvm::Module& module,
    llvm::LLVMContext& context
) {
    WrapperInfo info;
    info.originalName = func.getName().str();
    info.wrapperName = getWrapperName(info.originalName);
    info.success = false;
    
    // Check if function takes arguments
    if (func.arg_size() > 0) {
        info.errorMessage = "Cannot wrap function with arguments: " + info.originalName;
        return info;
    }
    
    // Get return type
    llvm::Type* returnType = func.getReturnType();
    
    // Check if we can convert the return type to i64
    if (!canConvertToI64(returnType, context)) {
        llvm::raw_string_ostream rso(info.errorMessage);
        rso << "Cannot convert return type to i64 for function: " << info.originalName;
        rso << " (type: " << *returnType << ")";
        return info;
    }
    
    // Create the wrapper function type: i64 ()
    llvm::Type* i64Type = llvm::Type::getInt64Ty(context);
    llvm::FunctionType* wrapperType = llvm::FunctionType::get(i64Type, false);
    
    // Create the wrapper function
    llvm::Function* wrapper = llvm::Function::Create(
        wrapperType,
        llvm::Function::ExternalLinkage,
        info.wrapperName,
        module
    );
    
    // Create the entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", wrapper);
    llvm::IRBuilder<> builder(entry);
    
    // Call the original function
    llvm::Value* callResult = builder.CreateCall(&func);
    
    // Convert the result to i64
    llvm::Value* i64Result = nullptr;
    
    if (returnType->isVoidTy()) {
        // Void function - return 0
        i64Result = llvm::ConstantInt::get(i64Type, 0);
    }
    else if (returnType->isIntegerTy()) {
        // Integer type - use ZExt or Trunc as needed
        llvm::IntegerType* intType = llvm::cast<llvm::IntegerType>(returnType);
        if (intType->getBitWidth() < 64) {
            i64Result = builder.CreateZExt(callResult, i64Type);
        } else if (intType->getBitWidth() > 64) {
            i64Result = builder.CreateTrunc(callResult, i64Type);
        } else {
            i64Result = callResult;
        }
    }
    else if (returnType->isPointerTy()) {
        // Pointer type - convert to i64 using PtrToInt
        i64Result = builder.CreatePtrToInt(callResult, i64Type);
    }
    else if (returnType->isFloatTy() || returnType->isDoubleTy()) {
        // Floating point - use bitcast to preserve bits
        // First convert to integer of same size, then extend to i64
        llvm::Type* intType = returnType->isFloatTy() 
            ? llvm::Type::getInt32Ty(context)
            : llvm::Type::getInt64Ty(context);
        llvm::Value* intVal = builder.CreateBitCast(callResult, intType);
        if (returnType->isFloatTy()) {
            i64Result = builder.CreateZExt(intVal, i64Type);
        } else {
            i64Result = intVal;
        }
    }
    else if (returnType->isStructTy() || returnType->isArrayTy()) {
        // For small structs/arrays, we need to extract the value
        // This is a simplified approach - just bitcast the first element
        // For a more complete solution, we'd need to handle each case
        
        // Try to bitcast through memory
        llvm::AllocaInst* alloca = builder.CreateAlloca(returnType);
        builder.CreateStore(callResult, alloca);
        
        llvm::Type* i64PtrType = llvm::PointerType::getUnqual(i64Type);
        llvm::Value* i64Ptr = builder.CreateBitCast(alloca, i64PtrType);
        i64Result = builder.CreateLoad(i64Type, i64Ptr);
    }
    else {
        // Unknown type - try to bitcast
        info.errorMessage = "Unsupported return type for function: " + info.originalName;
        wrapper->eraseFromParent();
        return info;
    }
    
    // Return the i64 result
    builder.CreateRet(i64Result);
    
    info.success = true;
    return info;
}

} // namespace llvm_repl
