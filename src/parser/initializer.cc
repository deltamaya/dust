//
// Created by delta on 14/03/2024.
//

#include "ast/expr.h"
#include "jit/dustjit.h"

namespace dust::parser{
    using namespace ast;
    std::unique_ptr<llvm::LLVMContext> TheContext;
    std::unique_ptr<llvm::IRBuilder<>> Builder;
    std::unique_ptr<llvm::Module> TheModule;
    std::map<std::string, std::pair<llvm::AllocaInst *,llvm::Type*>>  NamedValues;
    std::unique_ptr<DustJIT> TheJIT;
    std::unique_ptr<llvm::FunctionPassManager> TheFPM;
    std::unique_ptr<llvm::LoopAnalysisManager> TheLAM;
    std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
    std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM;
    std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
    std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
    std::unique_ptr<llvm::StandardInstrumentations> TheSI;
    std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
    llvm::ExitOnError ExitOnErr;
    
    void InitModuleAndManagers() {
        // Open a new context and module.
        TheContext = std::make_unique<llvm::LLVMContext>();
        TheModule = std::make_unique<llvm::Module>("DustJIT", *TheContext);
        TheModule->setDataLayout(TheJIT->getDataLayout());
        
        // Create a new builder for the module.
        Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
        
        // Create new pass and analysis managers.
        TheFPM = std::make_unique<llvm::FunctionPassManager>();
        TheLAM = std::make_unique<llvm::LoopAnalysisManager>();
        TheFAM = std::make_unique<llvm::FunctionAnalysisManager>();
        TheCGAM = std::make_unique<llvm::CGSCCAnalysisManager>();
        TheMAM = std::make_unique<llvm::ModuleAnalysisManager>();
        ThePIC = std::make_unique<llvm::PassInstrumentationCallbacks>();
        TheSI = std::make_unique<llvm::StandardInstrumentations>(*TheContext,
                /*DebugLogging*/ true);
        TheSI->registerCallbacks(*ThePIC, TheMAM.get());
        
        // Add transform passes.
// Do simple "peephole" optimizations and bit-twiddling optzns.
        TheFPM->addPass(llvm::InstCombinePass());
// Reassociate expressions.
        TheFPM->addPass(llvm::ReassociatePass());
// Eliminate Common SubExpressions.
        TheFPM->addPass(llvm::GVNPass());
// Simplify the control flow graph (deleting unreachable blocks, etc.).
        TheFPM->addPass(llvm::SimplifyCFGPass());// Register analysis passes used in these transform passes.

        llvm::PassBuilder PB;
        PB.registerModuleAnalyses(*TheMAM);
        PB.registerFunctionAnalyses(*TheFAM);
        PB.crossRegisterProxies(*TheLAM, *TheFAM, *TheCGAM, *TheMAM);
        
    }
    
}