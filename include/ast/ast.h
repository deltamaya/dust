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
#include <map>
#include "minilog.h"

static llvm::LLVMContext TheContext;

static llvm::IRBuilder<> Builder{TheContext};
static llvm::Module TheModule{"TheModule",TheContext};
static std::map<std::string, llvm::Value *> NamedValues;

class ExprAST {
public:
    virtual ~ExprAST() = default;
    
    virtual llvm::Value *codegen() = 0;
};

class NumberExprAST : public ExprAST {
    double val;
public:
    explicit NumberExprAST(double v) : val(v) {}
    
    inline llvm::Value *codegen() override;
};

inline llvm::Value *NumberExprAST::codegen() {
    return llvm::ConstantFP::get(TheContext, llvm::APFloat(val));
}

class StringExprAST : public ExprAST {
    std::string val;
public:
    explicit StringExprAST(std::string v) : val(std::move(v)) {}
    
    inline llvm::Value *codegen() override;
};

inline llvm::Value *StringExprAST::codegen() {
    // Look this variable up in the function.
//    llvm::StringLiteral s(llvm::StringRef(val));
//    if (!s)
//        return nullptr;
    return nullptr;
}


class VariableExprAST : public ExprAST {
    std::string name;
public:
    explicit VariableExprAST(std::string name) : name(std::move(name)) {}
    
    inline llvm::Value *codegen() override;
};

inline llvm::Value *VariableExprAST::codegen() {
    // Look this variable up in the function.
    llvm::Value *V = NamedValues[name];
    if (!V) {
        minilog::log_error("Unknown variable name: {}", name);
        return nullptr;
    }
    return V;
}

class BinaryExprAST : public ExprAST {
    lexer::Token op;
    std::unique_ptr<ExprAST> lhs, rhs;
public:
    BinaryExprAST(lexer::Token op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs) :
            op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    
    inline llvm::Value *codegen() override;
};

inline llvm::Value *BinaryExprAST::codegen() {
    llvm::Value *L = lhs->codegen();
    llvm::Value *R = rhs->codegen();
    if (!L || !R)
        return nullptr;
    
    switch (op.tok) {
        case lexer::ADD_TK:
            return Builder.CreateFAdd(L, R, "addtmp");
        case lexer::SUB_TK:
            return Builder.CreateFSub(L, R, "subtmp");
        case lexer::MUL_TK:
            return Builder.CreateFMul(L, R, "multmp");
        case lexer::DIV_TK:
            return Builder.CreateFDiv(L, R, "divtmp");
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
    
    inline llvm::Value *codegen() override;
};

inline llvm::Value *CallExprAST::codegen() {
    // Look up the name in the global module table.
    llvm::Function *CalleeF = TheModule.getFunction(callee);
    if (!CalleeF) {
        minilog::log_error("Unknown function name: {}", callee);
        return nullptr;
    }
    
    // If argument mismatch error.
    if (CalleeF->arg_size() != args.size()) {
        minilog::log_error("arguments mismatch");
        return nullptr;
    }
    
    std::vector<llvm::Value *> ArgsV;
    for (const auto &arg: args) {
        ArgsV.push_back(arg->codegen());
        if (!ArgsV.back())
            return nullptr;
    }
    
    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

class PrototypeAST : public ExprAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(std::string Name, std::vector<std::string> Args)
            : Name(std::move(Name)), Args(std::move(Args)) {}
    
    [[nodiscard]] std::string const &getName() { return Name; }
    
    inline llvm::Function *codegen() override;
};

inline llvm::Function *PrototypeAST::codegen() {
    // Make the function type:  double(double,double) etc.
    std::vector<llvm::Type *> Doubles(Args.size(),
                                      llvm::Type::getDoubleTy(TheContext));
    llvm::FunctionType *FT =
            llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);
    
    llvm::Function *F =
            llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, &TheModule);
    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg: F->args())
        Arg.setName(Args[Idx++]);
    minilog::log_info("add function declaration done: {}", Name);
    return F;
}

class FunctionAST : public ExprAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {}
    
    inline llvm::Function *codegen() override;
};

inline llvm::Function *FunctionAST::codegen() {
    minilog::log_info("function code gen: {}", Proto->getName());
    // First, check for an existing function from a previous 'extern' declaration.
    llvm::Function *TheFunction = TheModule.getFunction(Proto->getName());
    
    minilog::log_info("get name from module");
    if (!TheFunction) {
        minilog::log_info("doesn't contain such function prototype: {}",Proto->getName());
        TheFunction = Proto->codegen();
    }
    
    if (!TheFunction) {
        minilog::log_error("prototype codegen error");
        return nullptr;
    }
    
    if (!TheFunction->empty())
        return (llvm::Function *) nullptr;
    // Create a new basic block to start insertion into.
    minilog::log_info("generating function definition");
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", TheFunction);
    Builder.SetInsertPoint(BB);

// Record the function arguments in the NamedValues map.
    NamedValues.clear();
    for (auto &Arg: TheFunction->args())
        NamedValues[std::string(Arg.getName())] = &Arg;
    if (llvm::Value *RetVal = Body->codegen()) {
        // Finish off the function.
        Builder.CreateRet(RetVal);
        
        // Validate the generated code, checking for consistency.
        verifyFunction(*TheFunction);
        minilog::log_info("create function done");
        return TheFunction;
    }
    minilog::log_error("create function error");
    // Error reading body, remove function.
    TheFunction->eraseFromParent();
    return nullptr;
}

#endif //DUST_AST_H
