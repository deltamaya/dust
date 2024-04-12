//
// Created by delta on 12/04/2024.
//

#ifndef DUST_FUNC_H
#define DUST_FUNC_H
#include "expr.h"
#include "stmt.h"
namespace dust::ast{
    
    class FunctionAST : public ExprAST {
        std::unique_ptr<PrototypeAST> Proto;
        std::vector<std::unique_ptr<StmtAST>> Body;
    
    public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                    std::vector<std::unique_ptr<StmtAST>> Body)
                : Proto(std::move(Proto)), Body(std::move(Body)) {}
        
        llvm::Function *codegen() override;
    };
}
#endif //DUST_FUNC_H
