//
// Created by delta on 12/03/2024.
//

#ifndef DUST_EXPR_H
#define DUST_EXPR_H

#include <string>
#include <memory>
#include <utility>
#include <vector>
#include "lexer/lexer.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <map>
#include "utils/minilog.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "jit/dustjit.h"


namespace parser::ast{

    
    class ExprAST {
    public:
        virtual ~ExprAST() = default;
        
        virtual llvm::Value *codegen() = 0;
    };
    class StmtAST;
    class NumberExprAST : public ExprAST {
        double val;
    public:
        explicit NumberExprAST(double v) : val(v) {}
        
        llvm::Value *codegen() override;
    };
    
    
    class StringExprAST : public ExprAST {
        std::string val;
    public:
        explicit StringExprAST(std::string v) : val(std::move(v)) {}
        
        llvm::Value *codegen() override;
    };
    
    
    class PrototypeAST : public ExprAST {
        std::string Name;
        std::vector<std::string> Args;
    
    public:
        PrototypeAST(std::string Name, std::vector<std::string> Args)
                : Name(std::move(Name)), Args(std::move(Args)) {}
        
        [[nodiscard]] std::string const &getName() { return Name; }
        
        llvm::Function *codegen() override;
    };
    
    
    class VariableExprAST : public ExprAST {
        std::string name;
    public:
        explicit VariableExprAST(std::string name) : name(std::move(name)) {}
        [[nodiscard]] const std::string &getName() const { return name; }
        llvm::Value *codegen() override;
    };
    
    
    class BinaryExprAST : public ExprAST {
        lexer::Token op;
        std::unique_ptr<ExprAST> lhs, rhs;
    public:
        BinaryExprAST(lexer::Token op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs) :
                op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        
        llvm::Value *codegen() override;
    };
    
    
    class CallExprAST : public ExprAST {
        std::string callee;
        std::vector<std::unique_ptr<ExprAST>> args;
    public:
        CallExprAST(std::string Callee,
                    std::vector<std::unique_ptr<ExprAST>> Args)
                : callee(std::move(Callee)), args(std::move(Args)) {}
        
        llvm::Value *codegen() override;
    };
    
    class StmtAST{
    public:
        virtual ~StmtAST()=default;
        virtual void codegen()=0;
    };
    
    class FunctionAST : public ExprAST {
        std::unique_ptr<PrototypeAST> Proto;
        std::vector<std::unique_ptr<StmtAST>> Body;
    
    public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                    std::vector<std::unique_ptr<StmtAST>> Body)
                : Proto(std::move(Proto)), Body(std::move(Body)) {}
        
        llvm::Function *codegen() override;
    };
    
    class IfExprAST:public ExprAST{
        std::unique_ptr<ExprAST>Cond,Then,Else;
    public:
        IfExprAST(std::unique_ptr<ExprAST> Cond,  std::unique_ptr<ExprAST> Then,
                  std::unique_ptr<ExprAST> Else)
        : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
        
        llvm::Value *codegen() override;
    };
    class ForExprAST:public ExprAST{
        std::string VarName;
        std::unique_ptr<ExprAST> Start, End, Step, Body;
    
    public:
        ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
                   std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
                   std::unique_ptr<ExprAST> Body)
                : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
                  Step(std::move(Step)), Body(std::move(Body)) {}
        
        llvm::Value *codegen() override;
    };
    // VarExprAST - Expression class for var
    class VarExprAST : public ExprAST {
        std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
        std::unique_ptr<ExprAST> Body;
    
    public:
        VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
                   std::unique_ptr<ExprAST> Body)
                : VarNames(std::move(VarNames)), Body(std::move(Body)) {}
        
        llvm::Value *codegen() override;
    };
    
    
    class RegularStmtAST:public StmtAST{
    public:
        explicit RegularStmtAST(std::unique_ptr<ExprAST>val):val(std::move(val)){}
        std::unique_ptr<ExprAST> val;
        void codegen()override;
    };
    class ReturnStmtAST:public StmtAST{
    public:
        explicit ReturnStmtAST(std::unique_ptr<ExprAST>ret):retVal(std::move(ret)){}
        explicit ReturnStmtAST(std::unique_ptr<RegularStmtAST>reg):retVal(std::move(reg->val)){}
        std::unique_ptr<ExprAST> retVal;
        
        void codegen()override;
    };

}


#endif //DUST_EXPR_H
