//
// Created by delta on 14/03/2024.
//

#include "parser/parser.h"

namespace parser{
    void HandleFuncDef() {
        minilog::log_error("handle func def");
        if (auto fnAST = parseFuncDef()) {
            
            if (auto *fnIR = fnAST->codegen()) {
                fprintf(stdout, "Function Definition:");
                fnIR->print(llvm::errs());
                fprintf(stdout, "\n");
            }
            minilog::log_error("handle func def done");
            
        } else {
            minilog::log_error("error with func def");
            getNextToken();//skip token for error recovery
        }
        
    }
    
    void HandleTopLevelExpr() {
        // Evaluate a top-level expression into an anonymous function.
        if (auto FnAST = parseTopLevelExpr()) {
            if (auto *FnIR = FnAST->codegen()) {
                fprintf(stdout, "Read top-level expression:");
                FnIR->print(llvm::errs());
                fprintf(stdout, "\n");
                // Remove the anonymous expression.
                FnIR->eraseFromParent();
            }
        } else {
            // Skip token for error recovery.
            minilog::log_error("error with top level expr");
            getNextToken();
        }
    }
    
    void HandleExtern() {
        minilog::log_error("handle extern");
        if (auto proto = parseExtern()) {
            if (auto *protoIR = proto->codegen()) {
                fprintf(stdout, "Read top-level expression:");
                protoIR->print(llvm::errs());
                fprintf(stdout, "\n");
            }
            minilog::log_error("handle extern done");
        } else {
            minilog::log_error("error with extern");
            getNextToken();
        }
    }
}