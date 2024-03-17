//
// Created by delta on 16/03/2024.
//
#include "ast/ast.h"
#include "parser/parser.h"

namespace parser::ast{
    llvm::Value *NumberExprAST::codegen() {
        return llvm::ConstantFP::get(*TheContext, llvm::APFloat(val));
    }
    
    llvm::Value *StringExprAST::codegen() {
        // Look this variable up in the function.
//    llvm::StringLiteral s(llvm::StringRef(val));
//    if (!s)
//        return nullptr;
        return nullptr;
    }
    
    llvm::Value *VariableExprAST::codegen() {
        // Look this variable up in the function.
        llvm::Value *V = NamedValues[name];
        if (!V) {
            minilog::log_error("Unknown variable name: {}", name);
            return nullptr;
        }
        return V;
    }
    
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
    
    llvm::Value *CallExprAST::codegen() {
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
        minilog::log_info("add function declaration done: {}", Name);
        return F;
    }
    
    llvm::Function *FunctionAST::codegen() {
// Transfer ownership of the prototype to the FunctionProtos map, but keep a
        // reference to it for use below.
        auto &P = *Proto;
        FunctionProtos[Proto->getName()] = std::move(Proto);
        llvm::Function *TheFunction = getFunction(P.getName());
        if (!TheFunction)
            return nullptr;
        
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
            
            // Run the optimizer on the function.
            TheFPM->run(*TheFunction, *TheFAM);
            
            return TheFunction;
        }
        
        // Error reading body, remove function.
        TheFunction->eraseFromParent();
        return nullptr;
    }
    
    
    llvm::Function *getFunction(std::string Name) {
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
}