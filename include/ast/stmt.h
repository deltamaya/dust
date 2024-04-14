//
// Created by delta on 09/04/2024.
//

#ifndef DUST_STMT_H
#define DUST_STMT_H


#include "ast/expr.h"

namespace dust::ast{

    class StmtAST {
    public:
        virtual ~StmtAST() = default;
        
        virtual void codegen() = 0;
    };
    class RegularStmtAST : public StmtAST {
    public:
        explicit RegularStmtAST(std::unique_ptr<ExprAST> val) : val(std::move(val)) {}
        
        std::unique_ptr<ExprAST> val;
        
        void codegen() override;
    };
    
    class EmptyStmt : public StmtAST {
    public:
        void codegen() override;
    };
    
    class ReturnStmtAST : public StmtAST {
    public:
        explicit ReturnStmtAST(std::unique_ptr<ExprAST> ret) : retVal(std::move(ret)) {
        
        }
        
        explicit ReturnStmtAST(std::unique_ptr<RegularStmtAST> reg) : retVal(std::move(reg->val)) {}
        
        std::unique_ptr<ExprAST> retVal;
        
        void codegen() override;
    };
    
    class IfStmtAST : public StmtAST {
    public:
        IfStmtAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<StmtAST>> Then,
                  std::vector<std::unique_ptr<StmtAST>> Else) : Cond(std::move(Cond)), Then(std::move(Then)), Else
                (std::move(Else)) {}
        
        std::unique_ptr<ExprAST> Cond;
        std::vector<std::unique_ptr<StmtAST>> Then, Else;
        
        void codegen() override;
    };
    
    class ForStmtAST : public StmtAST {
    public:
        ForStmtAST(const std::string &varname, std::unique_ptr<ExprAST> Init, std::unique_ptr<ExprAST> Cond,
                   std::unique_ptr<ExprAST>
                   Then,
                   std::vector<std::unique_ptr<StmtAST>> Body) : Init(std::move(Init)), Cond(std::move(Cond)), Then
                (std::move(Then)),
                                                                 Body(std::move(Body)), VarName(varname) {}
        
        std::unique_ptr<ExprAST> Init;
        std::unique_ptr<ExprAST> Cond;
        std::unique_ptr<ExprAST> Then;
        std::vector<std::unique_ptr<StmtAST>> Body;
        std::string VarName;
        
        void codegen() override;
    };
    
    class VarStmtAST : public StmtAST {
    public:
        std::vector<std::pair<Variable,std::unique_ptr<ExprAST>>> vars;
        std::vector<std::unique_ptr<StmtAST>> Body;
        
        VarStmtAST(std::vector<std::pair<Variable,std::unique_ptr<ExprAST>>> vars,
                   std::vector<std::unique_ptr<StmtAST>> Body) : vars
                                                                         (std::move(vars)), Body(std::move(Body)) {}
        
        void codegen() override;
    };

    

}


#endif //DUST_STMT_H
