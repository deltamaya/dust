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

extern  std::unique_ptr<llvm::LLVMContext> TheContext;
extern  std::unique_ptr<llvm::IRBuilder<>> Builder;
extern  std::unique_ptr<llvm::Module> TheModule;
extern  std::map<std::string, llvm::Value *> NamedValues;


extern std::unique_ptr<DustJIT> TheJIT;
extern std::unique_ptr<llvm::FunctionPassManager> TheFPM;
extern std::unique_ptr<llvm::LoopAnalysisManager> TheLAM;
extern std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
extern std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM;
extern std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
extern std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
extern std::unique_ptr<llvm::StandardInstrumentations> TheSI;
class PrototypeAST;
extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
extern llvm::ExitOnError ExitOnErr;
void InitModuleAndManagers();
llvm::Function *getFunction(std::string Name);

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
    return llvm::ConstantFP::get(*TheContext, llvm::APFloat(val));
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
    
    inline llvm::Value *codegen() override;
};

inline llvm::Value *CallExprAST::codegen() {
    // Look up the name in the global module table.
    llvm::Function *CalleeF = getFunction(callee);
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
    
    return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
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
                                      llvm::Type::getDoubleTy(*TheContext));
    llvm::FunctionType *FT =
            llvm::FunctionType::get(llvm::Type::getDoubleTy(*TheContext), Doubles, false);
    
    llvm::Function *F =
            llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, TheModule.get());
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
// Transfer ownership of the prototype to the FunctionProtos map, but keep a
    // reference to it for use below.
    auto &P = *Proto;
    FunctionProtos[Proto->getName()] = std::move(Proto);
    llvm::Function *TheFunction = getFunction(P.getName());
    if (!TheFunction)
        return nullptr;
}

inline llvm::Function *getFunction(std::string Name) {
    // First, see if the function has already been added to the current module.
    if (auto *F = TheModule->getFunction(Name))
        return F;
    
    // If not, check whether we can codegen the declaration from some existing
    // prototype.
    auto FI = FunctionProtos.find(Name);
    if (FI != FunctionProtos.end())
        return FI->second->codegen();
    
    // If no existing prototype exists, return null.
    return nullptr;
}
#endif //DUST_AST_H
