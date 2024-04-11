//
// Created by delta on 09/04/2024.
//
#include "ast/stmt.h"
#include "parser/parser.h"

namespace parser::ast{
    
    
    void ReturnStmtAST::codegen() {
        // Generate code for the return value
        llvm::Value *RetVal = retVal->codegen();
        // Insert return instruction
        Builder->CreateRet(RetVal);
    }
    
    void RegularStmtAST::codegen() {
        val->codegen();
    }
    
    void EmptyStmt::codegen() {
    
    }
    
    void IfStmtAST::codegen() {
        llvm::Value *CondV = Cond->codegen();
        if (!CondV)
            return;
        
        // Convert condition to a bool by comparing non-equal to 0.0.
        CondV = Builder->CreateFCmpONE(
                CondV, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "ifcond");
        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
        
        // Create blocks for the then and else cases.
        llvm::BasicBlock *ThenBB =
                llvm::BasicBlock::Create(*TheContext, "then", TheFunction);
        llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(*TheContext, "else");
        llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(*TheContext, "afterif");
        
        // Create conditional branch based on the condition.
        Builder->CreateCondBr(CondV, ThenBB, ElseBB);
        
        // Emit then value.
        Builder->SetInsertPoint(ThenBB);
        for (const auto &stmt: Then) {
            stmt->codegen();
            // Check if there's already a terminator instruction, if so, don't generate code for the remaining statements.
            if (Builder->GetInsertBlock()->getTerminator()) {
                break;
            }
        }
        // Ensure we have a terminator in ThenBB.
        if (!Builder->GetInsertBlock()->getTerminator()) {
            Builder->CreateBr(MergeBB);
        }
        
        // Emit else block.
        TheFunction->insert(TheFunction->end(),ElseBB);
        Builder->SetInsertPoint(ElseBB);
        for (const auto &stmt: Else) {
            stmt->codegen();
            // Check if there's already a terminator instruction, if so, don't generate code for the remaining statements.
            if (Builder->GetInsertBlock()->getTerminator()) {
                break;
            }
        }
        // Ensure we have a terminator in ElseBB.
        if (!Builder->GetInsertBlock()->getTerminator()) {
            Builder->CreateBr(MergeBB);
        }
        
        // Emit merge block.
        TheFunction->insert(TheFunction->end(),MergeBB);
        Builder->SetInsertPoint(MergeBB);
    }
    
    void ForStmtAST::codegen() {
        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
        
        // Create an alloca for the variable in the entry block.
        llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
        
        // Emit the start code first, without 'variable' in scope.
        llvm::Value *StartVal = Init->codegen();
        if (!StartVal)
            return;
        
        // Store the value into the alloca.
        Builder->CreateStore(StartVal, Alloca);
        llvm::AllocaInst *OldVal = NamedValues[VarName];
        NamedValues[VarName] = Alloca;
        llvm::Value *StepVal;
        if (Then) {
            StepVal = Then->codegen();
        } else {
            StepVal = llvm::ConstantFP::get(llvm::Type::getDoubleTy(*TheContext), 1.0);
        }
        // Make the new basic block for the loop header, inserting after current
        // block.
        llvm::BasicBlock *CondBB =
                llvm::BasicBlock::Create(*TheContext, "cond", TheFunction);
        llvm::BasicBlock *LoopBB =
                llvm::BasicBlock::Create(*TheContext, "loop", TheFunction);
        llvm::BasicBlock *AfterBB =
                llvm::BasicBlock::Create(*TheContext, "afterloop", TheFunction);
        Builder->CreateBr(CondBB);
        Builder->SetInsertPoint(CondBB);
        llvm::Value *CondVal = Cond->codegen();
        CondVal = Builder->CreateFCmpONE(
                CondVal,
                llvm::ConstantFP::get(llvm::Type::getDoubleTy(*TheContext),
                                      llvm::APFloat(0.0)),
                "loopcond");
        
        Builder->CreateCondBr(CondVal, LoopBB, AfterBB);
        
        Builder->SetInsertPoint(LoopBB);
        for (const auto &stmt: Body) {
            stmt->codegen();
            // Check if there's already a terminator instruction, if so, don't generate code for the remaining statements.
            if (Builder->GetInsertBlock()->getTerminator()) {
                break;
            }
        }
        if(!Builder->GetInsertBlock()->getTerminator()){

            llvm::Value *CurVal =
                    Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, VarName.c_str());
            CurVal = Builder->CreateFAdd(CurVal, StepVal, "nextval");
            Builder->CreateStore(CurVal, Alloca);
            // Insert the conditional branch into the end of LoopEndBB.
            Builder->CreateBr(CondBB);
        }
        TheFunction->insert(TheFunction->end(),LoopBB);


        // Emit merge block.
//        TheFunction->insert(TheFunction->end(),AfterBB);
        // Any new code will be inserted in AfterBB.
        Builder->SetInsertPoint(AfterBB);
        // Restore the unshadowed variable.
        if (OldVal) {
            NamedValues[VarName] = OldVal;
        } else {
            NamedValues.erase(VarName);
        }
    }
}