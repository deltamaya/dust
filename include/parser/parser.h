//
// Created by delta on 13/03/2024.
//

#ifndef DUST_PARSER_H
#define DUST_PARSER_H
#include "ast/ast.h"
#include "lexer/lexer.h"
#include "minilog.h"
#include <map>


extern std::vector<lexer::Token> tokens;
extern lexer::Token curTok;
extern std::map<lexer::TokenId, int> BinOpPrecedence;

extern void getNextToken();
namespace parser{
    using uexpr = std::unique_ptr<ExprAST>;
    uexpr parseExpression();
    uexpr parseBinOpExpression(int exprPrec,uexpr lhs);
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
