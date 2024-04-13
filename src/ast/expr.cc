//
// Created by delta on 16/03/2024.
//
#include "ast/expr.h"
#include "parser/parser.h"

namespace dust::ast{
    using namespace parser;
    llvm::Value *NumberExprAST::codegen() {
        return llvm::ConstantFP::get(*TheContext, llvm::APFloat(val));
    }
    
    llvm::Value* StringExprAST::codegen() {
//        minilog::log_debug("string literal in std::string : {}",str);
        llvm::StringRef s{str};
//        minilog::log_debug("string literal in llvm::StringRef: {}",s.str());
        llvm::Value* stringPtr = Builder->CreateGlobalStringPtr(s, "string_literal");
        return stringPtr;
    }
    
    llvm::Value *VariableExprAST::codegen() {
        // Look this variable up in the function.
        llvm::AllocaInst *A = NamedValues[name].first;
        if (!A)
            return nullptr;
        
        // Load the value.
        return Builder->CreateLoad(A->getAllocatedType(), A, name.c_str());
    }
    
    llvm::Value *BinaryExprAST::codegen() {
        // Special case '=' because we don't want to emit the LHS as an expression.
        if (op.tok == lexer::ASSIGN_TK) {
            // Assignment requires the LHS to be an identifier.
            auto *LHSE = dynamic_cast<VariableExprAST *>(lhs.get());
            if (!LHSE)
                return nullptr;
            // Codegen the RHS.
            llvm::Value *Val = rhs->codegen();
            if (!Val)
                return nullptr;
            
            // Look up the name.
            llvm::Value *Variable = NamedValues[LHSE->getName()].first;
            if (!Variable)
                return nullptr;
            
            Builder->CreateStore(Val, Variable);
            return Val;
        }
        
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
            case lexer::LESS_TK:
                // this function will return a integer, 1 if L < R, 0 for else
                L = Builder->CreateFCmpULT(L, R, "cmptmp");
                // Convert the bool returned by CreateFCmpULT to double 0.0 or 1.0
                return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext),
                                             "booltmp");
            case lexer::LESSEQ_TK:
                L = Builder->CreateFCmpULE(L, R, "cmptmp");
                return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext),
                                             "booltmp");
            case lexer::GREATER_TK:
                L = Builder->CreateFCmpUGT(L, R, "cmptmp");
                return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext),
                                             "booltmp");
            case lexer::GREATEEQ_TK:
                L = Builder->CreateFCmpUGE(L, R, "cmptmp");
                return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext),
                                             "booltmp");
            case lexer::EQ_TK:
                L = Builder->CreateFCmpUEQ(L, R, "cmptmp");
                return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext),
                                             "booltmp");
            case lexer::NOTEQ_TK:
                L = Builder->CreateFCmpUNE(L, R, "cmptmp");
                return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext),
                                             "booltmp");
            default:
                minilog::log_fatal("can not parse operator: {}", lexer::to_string(op.tok));
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
        // this is function parameters
        //  Make the function type:  double(double,double) etc.
        std::vector<llvm::Type *> types;
        for(const auto&p:Args){
            types.push_back(getType(p.second));
        }
        
        // get the type of function by get, double(double ...)
        llvm::FunctionType *FT = llvm::FunctionType::get(
                getType(RetType), types, false);
        
        
        // create the function in the specific module
        llvm::Function *F = llvm::Function::Create(
                FT, llvm::Function::ExternalLinkage, Name, TheModule.get());
        // Set names for all arguments.
        unsigned Idx = 0;
        // set function parameter name
        for (auto &Arg: F->args())
            Arg.setName(Args[Idx++].first);
//        minilog::log_info("add function declaration done: {}", Name);
        return F;
    }
    
    
    
    llvm::Value *IfExprAST::codegen() {
        llvm::Value *CondV = Cond->codegen();
        if (!CondV)
            return nullptr;
        
        // Convert condition to a bool by comparing non-equal to 0.0.
        CondV = Builder->CreateFCmpONE(
                CondV, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "ifcond");
        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
        
        // Create blocks for the then and else cases.  Insert the 'then' block at the
        // end of the function.
        llvm::BasicBlock *ThenBB =
                llvm::BasicBlock::Create(*TheContext, "then", TheFunction);
        llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(*TheContext, "else");
        llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont");
        
        Builder->CreateCondBr(CondV, ThenBB, ElseBB);
        // Emit then value.
        Builder->SetInsertPoint(ThenBB);
        
        llvm::Value *ThenV = Then->codegen();
        if (!ThenV)
            return nullptr;
        
        Builder->CreateBr(MergeBB);
        // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
        ThenBB = Builder->GetInsertBlock();
        // Emit else block.
        TheFunction->insert(TheFunction->end(), ElseBB);
        Builder->SetInsertPoint(ElseBB);
        
        llvm::Value *ElseV = Else->codegen();
        if (!ElseV)
            return nullptr;
        
        Builder->CreateBr(MergeBB);
        // codegen of 'Else' can change the current block, update ElseBB for the PHI.
        ElseBB = Builder->GetInsertBlock();
        // Emit merge block.
        TheFunction->insert(TheFunction->end(), MergeBB);
        Builder->SetInsertPoint(MergeBB);
        llvm::PHINode *PN =
                Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, "iftmp");
        
        PN->addIncoming(ThenV, ThenBB);
        PN->addIncoming(ElseV, ElseBB);
        return PN;
    }
    

    
    
} // namespace parser::ast