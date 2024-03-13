//
// Created by delta on 12/03/2024.
//

#include "ast/ast.h"
#include "lexer/lexer.h"
extern std::vector<lexer::Token> tokens;
extern lexer::Token curTok;
extern void getNextToken();
namespace parser{
    
    std::unique_ptr<ExprAST> parse(){
        return nullptr;
    }
    std::unique_ptr<ExprAST> parseNumberExpr(){
        auto ret=std::make_unique<NumberExprAST>(std::stod(curTok.val));
        getNextToken();
        return ret;
    }
    
    
}