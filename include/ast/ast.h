//
// Created by delta on 12/03/2024.
//

#ifndef DUST_AST_H
#define DUST_AST_H

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

class ExprAST {
public:
    virtual ~ExprAST() = default;
    
    virtual llvm::Value *codegen() = 0;
};

static std::unique_ptr<llvm::LLVMContext> TheContext;

static std::unique_ptr<llvm::IRBuilder<>> Builder;//this is buggy
static std::unique_ptr<llvm::Module> TheModule;
static std::map<std::string, llvm::Value *> NamedValues;

class NumberExprAST : public ExprAST {
    double val;
public:
    explicit NumberExprAST(double v) : val(v) {}
    
    llvm::Value *codegen() override;
};

llvm::Value *NumberExprAST::codegen() {
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(val));
}

class StringExprAST : public ExprAST {
    std::string val;
public:
    explicit StringExprAST(std::string v) : val(std::move(v)) {}
    
    llvm::Value *codegen() override;
};


class VariableExprAST : public ExprAST {
    std::string name;
public:
    explicit VariableExprAST(std::string name) : name(std::move(name)) {}
    
    llvm::Value *codegen() override;
};

llvm::Value *VariableExprAST::codegen() {
    // Look this variable up in the function.
    llvm::Value *V = NamedValues[name];
    if (!V)
        return nullptr;
    return V;
}

class BinaryExprAST : public ExprAST {
    lexer::Token op;
    std::unique_ptr<ExprAST> lhs, rhs;
public:
    BinaryExprAST(lexer::Token op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs) :
            op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    
    llvm::Value *codegen() override;
};

llvm::Value *BinaryExprAST::codegen() {
    llvm::Value *L = lhs->codegen();
    llvm::Value *R = rhs->codegen();
    if (!L || !R)
        return nullptr;
    
    switch (op.tok) {
        case lexer::ADD_TK:
            return Builder->CreateFAdd(L, R, "addtmp");
        case lexer::SUB_TK:
            return Builder->CreateFSub(L, R, "subtmp");
        case lexer::MUL_TK:
            return Builder->CreateFMul(L, R, "multmp");
        case lexer::DIV_TK:
            return Builder->CreateFDiv(L, R, "divtmp");
        default:
            return nullptr;
    }
}

class CallExprAST : public ExprAST {
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;
public:
    CallExprAST(std::string Callee,
                std::vector<std::unique_ptr<ExprAST>> Args)
            : callee(std::move(Callee)), args(std::move(Args)) {}
    
    llvm::Value *codegen() override;
};

llvm::Value *CallExprAST::codegen() {
    // Look up the name in the global module table.
    llvm::Function *CalleeF = TheModule->getFunction(callee);
    if (!CalleeF)
        return nullptr;
    
    // If argument mismatch error.
    if (CalleeF->arg_size() != args.size())
        return nullptr;
    
    std::vector<llvm::Value *> ArgsV;
    for (const auto &arg: args) {
        ArgsV.push_back(arg->codegen());
        if (!ArgsV.back())
            return nullptr;
    }
    
    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

class PrototypeAST : public ExprAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(std::string Name, std::vector<std::string> Args)
            : Name(std::move(Name)), Args(std::move(Args)) {}
    
    [[nodiscard]] const std::string &getName() const { return Name; }
    
    llvm::Function *codegen() override;
};

llvm::Function *PrototypeAST::codegen() {
    // Make the function type:  double(double,double) etc.
    std::vector<llvm::Type *> Doubles(Args.size(),
                                      llvm::Type::getDoubleTy(*TheContext));
    llvm::FunctionType *FT =
            llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), Doubles, false);
    
    llvm::Function *F =
            llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, TheModule.get());
    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg: F->args())
        Arg.setName(Args[Idx++]);
    
    return F;
}

class FunctionAST : public ExprAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {}
    
    llvm::Function *codegen() override;
};

llvm::Function *FunctionAST::codegen() {
    // First, check for an existing function from a previous 'extern' declaration.
    llvm::Function *TheFunction = TheModule->getFunction(Proto->getName());
    
    if (!TheFunction)
        TheFunction = Proto->codegen();
    
    if (!TheFunction)
        return nullptr;
    
    if (!TheFunction->empty())
        return (llvm::Function *) nullptr;
    // Create a new basic block to start insertion into.
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", TheFunction);
    Builder->SetInsertPoint(BB);

// Record the function arguments in the NamedValues map.
    NamedValues.clear();
    for (auto &Arg: TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;
    if (llvm::Value *RetVal = Body->codegen()) {
        // Finish off the function.
        Builder->CreateRet(RetVal);
        
        // Validate the generated code, checking for consistency.
        verifyFunction(*TheFunction);
        
        return TheFunction;
    }
    // Error reading body, remove function.
    TheFunction->eraseFromParent();
    return nullptr;
}

#endif //DUST_AST_H
