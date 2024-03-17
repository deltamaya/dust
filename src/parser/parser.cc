//
// Created by delta on 12/03/2024.
//


#include "parser/parser.h"


namespace parser{
    using namespace minilog;
    
    bool isBinOperator(const lexer::Token &tk) {
        return tk.tok == lexer::ADD_TK ||
               tk.tok == lexer::SUB_TK ||
               tk.tok == lexer::MUL_TK ||
               tk.tok == lexer::DIV_TK;
    }
    
    int getTokPrecedence() {
        if (isBinOperator(curTok))
            return BinOpPrecedence[curTok.tok];
        else return -1;
    }
    
    uexpr MainLoop() {
        while (curTok.tok != lexer::EOF_TK) {
            if (curTok.tok == lexer::FN_TK) {
                HandleFuncDef();
            } else if (curTok.tok == lexer::EXTERN_TK) {
                HandleExtern();
            } else if (curTok.tok == lexer::SEMICON_TK) {
                getNextToken();
            } else {
                HandleTopLevelExpr();
            }
        }
        return nullptr;
    }
    
    std::unique_ptr<PrototypeAST> parseExtern() {
        getNextToken();//pass extern
        return parseFuncDecl();
    }
    
    uexpr parseNumberExpr() {
        auto ret = std::make_unique<NumberExprAST>(std::stod(curTok.val));
        getNextToken();
        log_info("num literal expr");
        return ret;
    }
    
    uexpr parseIdentifierExpr() {
        std::string name = curTok.val;
        getNextToken();//pass name
        if (curTok.tok != lexer::LPAR_TK) {
            log_info("ident expr");
            return std::make_unique<VariableExprAST>(name);
        }
        getNextToken();//pass (
        std::vector<uexpr> args;
        while (curTok.tok != lexer::RPAR_TK) {
            if (auto Arg = parseExpression();Arg)
                args.push_back(std::move(Arg));
            else
                return nullptr;
            if (curTok.tok != lexer::COMMA_TK) {
                if (curTok.tok == lexer::RPAR_TK) {
                    break;
                } else {
                    minilog::log_fatal("expect , or )");
                    std::exit(1);
                }
            }
            getNextToken();//pass ,
        }
        getNextToken();//pass )
        log_info("func call expr");
        return std::make_unique<CallExprAST>(name, std::move(args));
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
        log_info("() expr");
        return v;
    }
    
    uexpr parseStringExpr() {
        auto v = std::make_unique<StringExprAST>(curTok.val);
        getNextToken();//pass string literal
        log_info("string expr");
        return v;
    }
    
    uexpr parsePrimary() {
        if (curTok.tok == lexer::IDENT_TK) {
            return parseIdentifierExpr();
        } else if (curTok.tok == lexer::NUMLIT_TK) {
            return parseNumberExpr();
        } else if (curTok.tok == lexer::STR_TK) {
            return parseStringExpr();
        } else if (curTok.tok == lexer::LPAR_TK) {
            return parseParenthesisExpr();
        }
        return nullptr;
    }
    
    uexpr parseExpression() {
        
        auto l = parsePrimary();
        if (!l) { return nullptr; }
        return parseBinOpExpression(0, std::move(l));
    }
    
    uexpr parseBinOpExpression(int exprPrec, uexpr lhs) {
        while (true) {
            int tokPrec = getTokPrecedence();
            minilog::log_debug("tok {} precedence: {}", lexer::to_string(curTok.tok), tokPrec);
            if (tokPrec < exprPrec) {
                return lhs;
            }
            auto op = curTok;
            getNextToken();//pass bin op
            auto rhs = parsePrimary();
            if (!rhs) {
                return nullptr;
            }
            int nextPrec = getTokPrecedence();
            // if the next operator is prior,
            // let the current rhs expr bind with the next operator as its lhs
            if (tokPrec < nextPrec) {
                rhs = parseBinOpExpression(tokPrec + 1, std::move(rhs));
                if (!rhs)
                    return nullptr;
            }
            lhs = std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(rhs));
        }
    }
    
    std::unique_ptr<FunctionAST> parseTopLevelExpr() {
        if (auto e = parseExpression()) {
            auto proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
            getNextToken();//pass ;
            return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
        }
        return nullptr;
    }
    
    std::unique_ptr<PrototypeAST> parseFuncDecl() {
        
        std::string fnName = curTok.val;
        if (curTok.tok != lexer::IDENT_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        getNextToken();//pass name
        if (curTok.tok != lexer::LPAR_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        getNextToken();//pass (
        std::vector<std::string> argNames;
        while (curTok.tok != lexer::RPAR_TK) {
            argNames.push_back(curTok.val);
            if (curTok.tok != lexer::IDENT_TK) {
                log_fatal("fatal");
                std::exit(1);
            }
            getNextToken();//pass arg name
            if (curTok.tok == lexer::RPAR_TK) {
                break;
            }
            if (curTok.tok != lexer::COMMA_TK) {
                log_fatal("fatal");
                std::exit(1);
            }
            getNextToken();//pass ,
        }
        if (curTok.tok != lexer::RPAR_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        getNextToken();//pass )
        minilog::log_info("parsed func decl");
        return std::make_unique<PrototypeAST>(fnName, argNames);
    }
    
    std::unique_ptr<FunctionAST> parseFuncDef() {
        if (curTok.tok != lexer::FN_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        getNextToken();//pass fn
        auto signature = parseFuncDecl();
        if (curTok.tok != lexer::LBRACE_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        getNextToken();//pass {
        auto def = parseExpression();
        if (curTok.tok != lexer::RBRACE_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        getNextToken();//pass }
        minilog::log_info("parsed func def");
        return std::make_unique<FunctionAST>(std::move(signature), std::move(def));
    }
    
}