//
// Created by delta on 17/03/2024.
//
#include "parser/parser.h"
#include "ast/ast.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/IR/LegacyPassManager.h"
namespace emitter{
    int Emit(const std::string& filename){
        auto targetTriple=llvm::sys::getDefaultTargetTriple();
        minilog::log_debug("{}",targetTriple);
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
        std::string Error;
        auto Target = llvm::TargetRegistry::lookupTarget(targetTriple, Error);

// Print an error and exit if we couldn't find the requested target.
// This generally occurs if we've forgotten to initialise the
// TargetRegistry or we have a bogus target triple.
        if (!Target) {
            llvm::errs() << Error;
            return 1;
        }
        auto CPU = "generic";
        auto Features = "";
        
        llvm::TargetOptions opt;
        auto TargetMachine = Target->createTargetMachine(targetTriple, CPU, Features, opt, llvm::Reloc::PIC_);
        parser::TheModule->setDataLayout(TargetMachine->createDataLayout());
        parser::TheModule->setTargetTriple(targetTriple);
        auto Filename = filename;
        std::error_code EC;
        llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);
        
        if (EC) {
            llvm::errs() << "Could not open file: " << EC.message();
            return 1;
        }
        llvm::legacy::PassManager pass;
        auto FileType = llvm::CodeGenFileType::ObjectFile;
        
        if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
            llvm::errs() << "TargetMachine can't emit a file of this type";
            return 1;
        }
        
        pass.run(*parser::TheModule);
        dest.flush();
        return 0;
    }
}