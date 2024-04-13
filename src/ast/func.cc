//
// Created by delta on 12/04/2024.
//

#include "ast/func.h"
#include "parser/parser.h"
namespace dust::ast{
    using namespace parser;
    llvm::Function *FunctionAST::codegen() {
        auto &P = *Proto;
        FunctionProtos[Proto->getName()] = std::move(Proto);
        llvm::Function *TheFunction = getFunction(P.getName());
        if (!TheFunction)
            return nullptr;
        
        llvm::BasicBlock *EntryBB =
                llvm::BasicBlock::Create(*TheContext, "entry", TheFunction);
        Builder->SetInsertPoint(EntryBB);
        
        NamedValues.clear();
        for (auto &Arg: TheFunction->args()) {
            llvm::AllocaInst *Alloca =
                    CreateEntryBlockAlloca(TheFunction,Arg.getType(), std::string{Arg.getName()});
            Builder->CreateStore(&Arg, Alloca);
            NamedValues[std::string(Arg.getName())] = {Alloca,Arg.getType()};
        }
        
        // Generate code for each statement in the function body
        for (const auto &Stmt: Body) {
            Stmt->codegen();
            // Check if there's already a terminator instruction, if so, don't generate code for the remaining statements
            if (Builder->GetInsertBlock()->getTerminator()) {
                break;
            }
        }
        
        if (!Builder->GetInsertBlock()->getTerminator()) {
            // If no return statement is encountered, create a default return of void
            Builder->CreateRetVoid();
        }
        
        if (verifyFunction(*TheFunction)) {
            TheFunction->eraseFromParent();
            minilog::log_error("function definition error");
            std::fflush(stderr);
            return nullptr;
        }
        
        TheFPM->run(*TheFunction, *TheFAM);
        return TheFunction;
    }
    
}