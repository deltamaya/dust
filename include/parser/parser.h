//
// Created by delta on 13/03/2024.
//

#ifndef DUST_PARSER_H
#define DUST_PARSER_H

#include "ast/ast.h"
#include "lexer/lexer.h"
#include "utils/minilog.h"
#include <map>

// these are defined in main.cc
extern std::vector<lexer::Token> tokens;
extern lexer::Token curTok;
extern std::map<lexer::TokenId, int> BinOpPrecedence;


extern void getNextToken();

namespace parser{
    using namespace parser::ast;
    using uexpr = std::unique_ptr<ast::ExprAST>;
    // these are defined in initializer.cc
    extern std::unique_ptr<llvm::LLVMContext> TheContext;
    extern std::unique_ptr<llvm::IRBuilder<>> Builder;
    extern std::unique_ptr<llvm::Module> TheModule;
    extern std::map<std::string, llvm::Value *> NamedValues;
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
    
    void InitModuleAndManagers();
    
    uexpr parseExpression();
    
    uexpr parseBinOpExpression(int exprPrec, uexpr lhs);
    
    std::unique_ptr<PrototypeAST> parseFuncDecl();
    
    std::unique_ptr<FunctionAST> parseFuncDef();
    
    std::unique_ptr<FunctionAST> parseTopLevelExpr();
    
    std::unique_ptr<PrototypeAST> parseExtern();
    
    uexpr MainLoop();
    
    void HandleFuncDef();
    
    void HandleTopLevelExpr();
    
    void HandleExtern();
}
#endif //DUST_PARSER_H
