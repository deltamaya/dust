//
// Created by delta on 13/03/2024.
//

#ifndef DUST_PARSER_H
#define DUST_PARSER_H

#include "ast/expr.h"
#include "ast/stmt.h"
#include "lexer/lexer.h"
#include "utils/minilog.h"
#include <map>
#include "ast/func.h"
using namespace dust;




namespace dust::parser{
    using namespace ast;
    using uexpr = std::unique_ptr<ast::ExprAST>;
    // these are defined in initializer.cc
    extern std::unique_ptr<llvm::LLVMContext> TheContext;
    extern std::unique_ptr<llvm::IRBuilder<>> Builder;
    extern std::unique_ptr<llvm::Module> TheModule;
    extern std::map<std::string, std::pair<llvm::AllocaInst *,llvm::Type*>> NamedValues;
    extern std::unique_ptr<DustJIT> TheJIT;
    extern std::unique_ptr<llvm::FunctionPassManager> TheFPM;
    extern std::unique_ptr<llvm::LoopAnalysisManager> TheLAM;
    extern std::unique_ptr<llvm::FunctionAnalysisManager> TheFAM;
    extern std::unique_ptr<llvm::CGSCCAnalysisManager> TheCGAM;
    extern std::unique_ptr<llvm::ModuleAnalysisManager> TheMAM;
    extern std::unique_ptr<llvm::PassInstrumentationCallbacks> ThePIC;
    extern std::unique_ptr<llvm::StandardInstrumentations> TheSI;
    extern std::map<std::string, std::unique_ptr<ast::PrototypeAST>> FunctionProtos;
    extern llvm::ExitOnError ExitOnErr;
    //defined in parser.cc
    extern std::map<lexer::TokenId, int> BinOpPrecedence;
    extern std::function<void()> PassToken;
    extern std::function<lexer::Token()>GetToken;
    void InitModuleAndManagers();
    llvm::Function *getFunction(std::string const& Name);
    llvm::Type* getType(lexer::TokenId t);
    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction,llvm::Type*,
                                             const std::string &VarName);
    uexpr parseExpression();
    
    uexpr parseBinOpExpression(int exprPrec, uexpr lhs);
    
    std::unique_ptr<PrototypeAST> parseFuncDecl();
    
    std::unique_ptr<FunctionAST> parseFuncDef();
    
    std::unique_ptr<FunctionAST> parseTopLevelExpr();
    
    std::unique_ptr<PrototypeAST> parseExtern();
    std::unique_ptr<ExprAST> parseIfExpr();
    enum ParseMode{
        Interactive=0,
        File
    };
    void SetParseMode(ParseMode m);
    uexpr MainLoop();
    
    void InterpretFuncDef();
    
    void InterpretTopLevelExpr();
    
    void InterpretExtern();
}
#endif //DUST_PARSER_H
