//
// Created by delta on 09/04/2024.
//
#include "ast/stmt.h"
#include "parser/parser.h"

namespace dust::ast{
    using namespace parser;
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
        llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction,getType(lexer::NUM_TK), VarName);
        
        // Emit the start code first, without 'variable' in scope.
        llvm::Value *StartVal = Init->codegen();
        if (!StartVal)
            return;
        
        // Store the value into the alloca.
        Builder->CreateStore(StartVal, Alloca);
        llvm::AllocaInst *OldVal = NamedValues[VarName].first;
        NamedValues[VarName] = {Alloca,getType(lexer::NUM_TK)};
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
        // Any new code will be inserted in AfterBB.
        Builder->SetInsertPoint(AfterBB);
        // Restore the unshadowed variable.
        if (OldVal) {
            NamedValues[VarName] = {OldVal,getType(lexer::NUM_TK)};
        } else {
            NamedValues.erase(VarName);
        }
    }
    
    void VarStmtAST::codegen() {
        std::vector<std::pair<llvm::AllocaInst *,llvm::Type*>> OldBindings;
        
        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* VarBB=llvm::BasicBlock::Create(*TheContext,"varBB",TheFunction);
        Builder->CreateBr(VarBB);
        Builder->SetInsertPoint(VarBB);
        // Register all variables and emit their initializer.
        for (const auto &p: vars) {
            auto & Var=p.first;
            auto& Val=p.second;
            ExprAST *Init =Val.get();
            
            // Emit the initializer before adding the variable to scope, this prevents
            // the initializer from referencing the variable itself, and permits stuff
            llvm::Value *InitVal;
            if (Init) {
                InitVal = Init->codegen();
            } else { // If not specified, use 0.0.
                InitVal = llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0));
            }
            llvm::Type* type= getType(Var.typeId);
            llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction,type, Var.name);
            Builder->CreateStore(InitVal, Alloca);
            
            // Remember the old variable binding so that we can restore the binding when
            // we unrecurse.
            OldBindings.emplace_back(NamedValues[ Var.name].first,type);
            
            // Remember this binding.
            NamedValues[ Var.name] = {Alloca,type};
        }
        
        // Codegen the body, now that all vars are in scope.
        for(const auto&stmt:Body){
            stmt->codegen();
            if(Builder->GetInsertBlock()->getTerminator()){
                break;
            }
        }
        // Pop all our variables from scope.
        for (unsigned i = 0, e = vars.size(); i != e; ++i)
            NamedValues[vars[i].first.name] = OldBindings[i];
    }
}