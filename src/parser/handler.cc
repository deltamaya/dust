//
// Created by delta on 14/03/2024.
//

#include "parser/parser.h"
#include "ast/expr.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"

namespace parser{
    
    void InterpretFuncDef() {
        minilog::log_info("handle func def");
        if (auto fnAST = parseFuncDef()) {
            
            if (auto *fnIR = fnAST->codegen()) {
                fprintf(stderr, "Read function definition:");
                fnIR->print(llvm::errs());
                fprintf(stderr, "\n");
                ExitOnErr(TheJIT->addModule(
                        ThreadSafeModule(std::move(TheModule), std::move(TheContext))));
                InitModuleAndManagers();
            }else{
                minilog::log_fatal("handle func error");
                std::exit(10);
            }

            minilog::log_info("handle func def done");
            
        } else {
            minilog::log_info("error with func def");
            passToken();//skip token for error recovery
        }
        
    }
    
    void InterpretTopLevelExpr() {
        // Evaluate a top-level expression into an anonymous function.
        if (auto FnAST = parseTopLevelExpr()) {
            if (FnAST->codegen()) {
                // Create a ResourceTracker to track JIT'd memory allocated to our
                // anonymous expression -- that way we can free it after executing.
                auto RT = TheJIT->getMainJITDylib().createResourceTracker();
                
                auto TSM = llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext));
                ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
                InitModuleAndManagers();
                
                // Search the JIT for the __anon_expr symbol.
                auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));
//                assert(ExprSymbol);
                
                // Get the symbol's address and cast it to the right type (takes no
                // arguments, returns a double) so we can call it as a native function.
                double (*FP)() = ExprSymbol.getAddress().toPtr < double(*)
                () > ();
                fprintf(stderr, "Evaluated to %f\n", FP());
                
                // Delete the anonymous expression module from the JIT.
                ExitOnErr(RT->remove());
            }
        } else {
            // Skip token for error recovery.
            minilog::log_info("error with top level expr");
            passToken();
        }
    }
    
    void InterpretExtern() {
        minilog::log_info("handle extern");
        if (auto proto = parseExtern()) {
            if (auto *protoIR = proto->codegen()) {
                fprintf(stderr, "Read top-level expression:");
                protoIR->print(llvm::errs());
                fprintf(stderr, "\n");
                FunctionProtos[proto->getName()] = std::move(proto);
            }
            minilog::log_info("handle extern done");
        } else {
            minilog::log_info("error with extern");
            passToken();
        }
    }
    
    void InterpretIfExpr(){
    
    }
    void InterpretForExpr(){
    
    }
    
    void CompileFuncDef() {
        minilog::log_info("handle func def");
        if (auto fnAST = parseFuncDef()) {
            
            if (auto *fnIR = fnAST->codegen()) {
                fprintf(stderr, "Read function definition:");
                fnIR->print(llvm::errs());
                fprintf(stderr, "\n");
            }
            minilog::log_info("handle func def done");
            
        } else {
            minilog::log_info("error with func def");
            passToken();//skip token for error recovery
        }
    }
    
    void CompileTopLevelExpr() {
        // Evaluate a top-level expression into an anonymous function.
        if (auto FnAST = parseTopLevelExpr()) {
            FnAST->codegen();
        } else {
            // Skip token for error recovery.
            minilog::log_info("error with top level expr");
            passToken();
        }
    }
    
    void CompileExtern() {
        minilog::log_info("handle extern");
        if (auto proto = parseExtern()) {
            if (auto *protoIR = proto->codegen()) {
                fprintf(stderr, "Read top-level expression:");
                protoIR->print(llvm::errs());
                fprintf(stderr, "\n");
                FunctionProtos[proto->getName()] = std::move(proto);
            }
            minilog::log_info("handle extern done");
        } else {
            minilog::log_info("error with extern");
            passToken();
        }
    }
}