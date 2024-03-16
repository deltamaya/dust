//
// Created by delta on 16/03/2024.
//

#ifndef DUST_DUSTJIT_H
#define DUST_DUSTJIT_H

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include <memory>


using namespace llvm::orc;

class DustJIT {


private:
    std::unique_ptr<llvm::orc::ExecutionSession> ES;
    
    llvm::DataLayout DL;
    MangleAndInterner Mangle;
    
    RTDyldObjectLinkingLayer ObjectLayer;
    IRCompileLayer CompileLayer;
    
    JITDylib &MainJD;

public:
    DustJIT(std::unique_ptr<ExecutionSession> ES,
            JITTargetMachineBuilder JTMB, llvm::DataLayout DL)
            : ES(std::move(ES)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
              ObjectLayer(*this->ES,
                          []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
              CompileLayer(*this->ES, ObjectLayer,
                           std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
              MainJD(this->ES->createBareJITDylib("<main>")) {
        MainJD.addGenerator(
                cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
                        DL.getGlobalPrefix())));
        if (JTMB.getTargetTriple().isOSBinFormatCOFF()) {
            ObjectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
            ObjectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
        }
    }
    
    ~DustJIT() {
        if (auto Err = ES->endSession())
            ES->reportError(std::move(Err));
    }
    
    static std::unique_ptr<DustJIT> Create() {
        auto EPC = SelfExecutorProcessControl::Create();
        if (!EPC)
            return nullptr;
        
        auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));
        
        JITTargetMachineBuilder JTMB(
                ES->getExecutorProcessControl().getTargetTriple());
        
        auto DL = JTMB.getDefaultDataLayoutForTarget();
        if (!DL)
            return nullptr;
        
        return std::make_unique<DustJIT>(std::move(ES), std::move(JTMB),
                                         std::move(*DL));
    }
    
    const llvm::DataLayout &getDataLayout() const { return DL; }
    
    JITDylib &getMainJITDylib() { return MainJD; }
    
    llvm::Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr) {
        if (!RT)
            RT = MainJD.getDefaultResourceTracker();
        return CompileLayer.add(RT, std::move(TSM));
    }
    
    llvm::Expected<ExecutorSymbolDef> lookup(llvm::StringRef Name) {
        return ES->lookup({&MainJD}, Mangle(Name.str()));
    }
};


#endif //DUST_DUSTJIT_H
