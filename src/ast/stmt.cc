//
// Created by delta on 09/04/2024.
//
#include "ast/stmt.h"
#include "parser/parser.h"

namespace parser::ast{

    
    void ReturnStmtAST::codegen() {
        if (llvm::Value *RetVal = retVal->codegen()) {
            // Finish off the function.
            Builder->CreateRet(RetVal);
        }
    }
    void RegularStmtAST::codegen() {
        val->codegen();
    }
}