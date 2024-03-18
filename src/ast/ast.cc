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
        llvm::AllocaInst *A = NamedValues[name];
        if (!A)
            return nullptr;
        
        // Load the value.
        return Builder->CreateLoad(A->getAllocatedType(), A, name.c_str());
    }
    
    llvm::Value *BinaryExprAST::codegen() {
        // Special case '=' because we don't want to emit the LHS as an expression.
        if (op.tok == lexer::ASSIGN_TK) {
            // Assignment requires the LHS to be an identifier.
            // This assume we're building without RTTI because LLVM builds that way by
            // default.  If you build LLVM with RTTI this can be changed to a
            // dynamic_cast for automatic error checking.
            auto *LHSE = dynamic_cast<VariableExprAST *>(lhs.get());
            if (!LHSE)
                return nullptr;
            // Codegen the RHS.
            llvm::Value *Val = rhs->codegen();
            if (!Val)
                return nullptr;
            
            // Look up the name.
            llvm::Value *Variable = NamedValues[LHSE->getName()];
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
                L = Builder->CreateFCmpULT(L, R, "cmptmp");
                // Convert bool 0/1 to double 0.0 or 1.0
                return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext), "booltmp");
            default:
                minilog::log_fatal("can not parse operator: {}",lexer::to_string(op.tok));
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
        for (auto &Arg : TheFunction->args()) {
            // Create an alloca for this variable.
            llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, std::string{Arg.getName()});
            
            // Store the initial value into the alloca.
            Builder->CreateStore(&Arg, Alloca);
            
            // Add arguments to variable symbol table.
            NamedValues[std::string(Arg.getName())] = Alloca;
        }
        
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
    llvm::Value* IfExprAST::codegen(){
        llvm::Value *CondV = Cond->codegen();
        if (!CondV)
            return nullptr;
        
        // Convert condition to a bool by comparing non-equal to 0.0.
        CondV = Builder->CreateFCmpONE(
                CondV, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)), "ifcond");
        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();

// Create blocks for the then and else cases.  Insert the 'then' block at the
// end of the function.
        llvm:: BasicBlock *ThenBB =
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
    
    llvm::Value *ForExprAST::codegen() {
        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
        
        // Create an alloca for the variable in the entry block.
        llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
        
        // Emit the start code first, without 'variable' in scope.
        llvm::Value *StartVal = Start->codegen();
        if (!StartVal)
            return nullptr;
        
        // Store the value into the alloca.
        Builder->CreateStore(StartVal, Alloca);
        
        // Make the new basic block for the loop header, inserting after current
        // block.
        llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(*TheContext, "loop", TheFunction);
        
        // Insert an explicit fall through from the current block to the LoopBB.
        Builder->CreateBr(LoopBB);
        
        // Start insertion in LoopBB.
        Builder->SetInsertPoint(LoopBB);
        
        // Within the loop, the variable is defined equal to the PHI node.  If it
        // shadows an existing variable, we have to restore it, so save it now.
        llvm::AllocaInst *OldVal = NamedValues[VarName];
        NamedValues[VarName] = Alloca;
        
        // Emit the body of the loop.  This, like any other expr, can change the
        // current BB.  Note that we ignore the value computed by the body, but don't
        // allow an error.
        if (!Body->codegen())
            return nullptr;
        
        // Emit the step value.
        llvm::Value *StepVal = nullptr;
        if (Step) {
            StepVal = Step->codegen();
            if (!StepVal)
                return nullptr;
        } else {
            // If not specified, use 1.0.
            StepVal = llvm::ConstantFP::get(*TheContext, llvm::APFloat(1.0));
        }
        
        // Compute the end condition.
        llvm:: Value *EndCond = End->codegen();
        if (!EndCond)
            return nullptr;
        
        // Reload, increment, and restore the alloca.  This handles the case where
        // the body of the loop mutates the variable.
        llvm::Value *CurVar =
                Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, VarName.c_str());
        llvm::Value *NextVar = Builder->CreateFAdd(CurVar, StepVal, "nextvar");
        Builder->CreateStore(NextVar, Alloca);
        
        // Convert condition to a bool by comparing non-equal to 0.0.
        EndCond = Builder->CreateFCmpONE(
                EndCond, llvm::ConstantFP::get(*TheContext,llvm::APFloat(0.0)), "loopcond");
        
        // Create the "after loop" block and insert it.
        llvm::BasicBlock *AfterBB =
                llvm::BasicBlock::Create(*TheContext, "afterloop", TheFunction);
        
        // Insert the conditional branch into the end of LoopEndBB.
        Builder->CreateCondBr(EndCond, LoopBB, AfterBB);
        
        // Any new code will be inserted in AfterBB.
        Builder->SetInsertPoint(AfterBB);
        
        // Restore the unshadowed variable.
        if (OldVal)
            NamedValues[VarName] = OldVal;
        else
            NamedValues.erase(VarName);
        
        // for expr always returns 0.0.
        return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*TheContext));
    }
            

    llvm::Value* VarExprAST::codegen(){
        std::vector<llvm::AllocaInst *> OldBindings;
        
        llvm::Function *TheFunction = Builder->GetInsertBlock()->getParent();
        
        // Register all variables and emit their initializer.
        for (auto & i : VarNames) {
            const std::string &VarName = i.first;
            ExprAST *Init = i.second.get();
            
            // Emit the initializer before adding the variable to scope, this prevents
            // the initializer from referencing the variable itself, and permits stuff
            // like this:
            //  var a = 1 in
            //    var a = a in ...   # refers to outer 'a'.
            llvm::Value *InitVal;
            if (Init) {
                InitVal = Init->codegen();
                if (!InitVal)
                    return nullptr;
            } else { // If not specified, use 0.0.
                InitVal = llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0));
            }
            
            llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
            Builder->CreateStore(InitVal, Alloca);
            
            // Remember the old variable binding so that we can restore the binding when
            // we unrecurse.
            OldBindings.push_back(NamedValues[VarName]);
            
            // Remember this binding.
            NamedValues[VarName] = Alloca;
        }
        
        // Codegen the body, now that all vars are in scope.
        llvm::Value *BodyVal = Body->codegen();
        if (!BodyVal)
            return nullptr;
        
        // Pop all our variables from scope.
        for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
            NamedValues[VarNames[i].first] = OldBindings[i];
        
        // Return the body computation.
        return BodyVal;
    };
    
}