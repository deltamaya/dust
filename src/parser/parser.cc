//
// Created by delta on 12/03/2024.
//

#include "ast/ast.h"
#include "lexer/lexer.h"
#include "minilog.h"

extern std::vector<lexer::Token> tokens;
extern lexer::Token curTok;

extern void getNextToken();

namespace parser{
    using uexpr = std::unique_ptr<ExprAST>;
    
    uexpr parse() {
        curTok = tokens[0];
        while (curTok.tok != lexer::EOF_TK) {
            if (curTok.tok == lexer::FN_TK) {
            }
        }
    }
    
    uexpr parseExpression();
    
    uexpr parseNumberExpr() {
        auto ret = std::make_unique<NumberExprAST>(std::stod(curTok.val));
        getNextToken();
        return ret;
    }
    
    uexpr parseIdentifierExpr() {
        std::string name = curTok.val;
        getNextToken();//pass name
        if (curTok.tok != lexer::LPAR_TK) {
            return std::make_unique<VariableExprAST>(name);
        }
        getNextToken();//pass (
        std::vector<uexpr> args;
        while (curTok.tok != lexer::RPAR_TK) {
            if (auto Arg = parseExpression();Arg)
                args.push_back(std::move(Arg));
            else
                return nullptr;
            if (curTok.tok == lexer::COMMA_TK)
                getNextToken();//pass ,
        }
        getNextToken();//pass )
        return std::make_unique<CallExprAST>(name,std::move(args));
    }
    
    uexpr parseParenthesisExpr() {
        getNextToken();//pass '('
        auto v = parseExpression();
        if (!v) {
            return nullptr;
        }
        if (curTok.tok != lexer::RPAR_TK) {
            minilog::log_fatal("can not parse parenthesis expression");
            std::exit(-1);
        }
        getNextToken();//pass ')'
        return v;
    }
    
    uexpr parseStringExpr(){
        auto v=std::make_unique<StringExprAST>(curTok.val);
        getNextToken();//pass string literal
        return v;
    }
    
    uexpr parseExpression(){
        if(curTok.tok==lexer::IDENT_TK){
            parseIdentifierExpr();
        }else if(curTok.tok==lexer::NUMLIT_TK){
            parseNumberExpr();
        }else if(curTok.tok==lexer::STR_TK){
            parseStringExpr();
        }
    }
    
}