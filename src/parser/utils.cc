//
// Created by delta on 18/03/2024.
//
#include "ast/expr.h"
#include "parser/parser.h"

namespace dust::parser{
// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
// the function.  This is used for mutable variables etc.
    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction,
                                             const std::string &VarName) {
        llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                               TheFunction->getEntryBlock().begin());
        return TmpB.CreateAlloca(llvm::Type::getDoubleTy(*TheContext), nullptr,
                                 VarName);
    }
    
    llvm::Function *getFunction(const std::string &Name) {
        // First, see if the function has already been added to the current module.
        if (auto *F = TheModule->getFunction(Name))
            return F;
        
        // If not, check whether we can codegen the declaration from some existing
        // prototype.
        auto FI = FunctionProtos.find(Name);
        if (FI != FunctionProtos.end())
            return FI->second->codegen();
        
        // If no existing prototype exists, return null.
        return nullptr;
    }
    
} // namespace parser