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
        if (isBinOperator(getToken()))
            return BinOpPrecedence[getToken().tok];
        else return -1;
    }
    
    uexpr MainLoop() {
        while (getToken().tok != lexer::EOF_TK) {
            if (getToken().tok == lexer::FN_TK) {
                CompileFuncDef();
            } else if (getToken().tok == lexer::EXTERN_TK) {
                CompileExtern();
            } else if (getToken().tok == lexer::SEMICON_TK) {
                passToken();
            } else {
                CompileTopLevelExpr();
            }
        }
        return nullptr;
    }
    
    std::unique_ptr<PrototypeAST> parseExtern() {
        passToken();//pass extern
        return parseFuncDecl();
    }
    
    uexpr parseNumberExpr() {
        auto ret = std::make_unique<NumberExprAST>(std::stod(getToken().val));
        passToken();
        log_info("num literal expr");
        return ret;
    }
    
    uexpr parseIdentifierExpr() {
        std::string name = getToken().val;
        passToken();//pass name
        if (getToken().tok != lexer::LPAR_TK) {
            log_info("ident expr");
            return std::make_unique<VariableExprAST>(name);
        }
        passToken();//pass (
        std::vector<uexpr> args;
        while (getToken().tok != lexer::RPAR_TK) {
            if (auto Arg = parseExpression();Arg)
                args.push_back(std::move(Arg));
            else
                return nullptr;
            if (getToken().tok != lexer::COMMA_TK) {
                if (getToken().tok == lexer::RPAR_TK) {
                    break;
                } else {
                    minilog::log_fatal("expect , or )");
                    std::exit(1);
                }
            }
            passToken();//pass ,
        }
        passToken();//pass )
        log_info("func call expr");
        return std::make_unique<CallExprAST>(name, std::move(args));
    }
    
    uexpr parseParenthesisExpr() {
        passToken();//pass '('
        auto v = parseExpression();
        if (!v) {
            return nullptr;
        }
        if (getToken().tok != lexer::RPAR_TK) {
            minilog::log_fatal("can not parse parenthesis expression");
            std::exit(-1);
        }
        passToken();//pass ')'
        log_info("() expr");
        return v;
    }
    
    uexpr parseStringExpr() {
        auto v = std::make_unique<StringExprAST>(getToken().val);
        passToken();//pass string literal
        log_info("string expr");
        return v;
    }
    
    uexpr parsePrimary() {
        if (getToken().tok == lexer::IDENT_TK) {
            return parseIdentifierExpr();
        } else if (getToken().tok == lexer::NUMLIT_TK) {
            return parseNumberExpr();
        } else if (getToken().tok == lexer::STR_TK) {
            return parseStringExpr();
        } else if (getToken().tok == lexer::LPAR_TK) {
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
            minilog::log_debug("tok {} precedence: {}", lexer::to_string(getToken().tok), tokPrec);
            if (tokPrec < exprPrec) {
                return lhs;
            }
            auto op = getToken();
            passToken();//pass bin op
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
            passToken();//pass ;
            return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
        }
        return nullptr;
    }
    
    std::unique_ptr<PrototypeAST> parseFuncDecl() {
        
        std::string fnName = getToken().val;
        if (getToken().tok != lexer::IDENT_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        passToken();//pass name
        if (getToken().tok != lexer::LPAR_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        passToken();//pass (
        std::vector<std::string> argNames;
        while (getToken().tok != lexer::RPAR_TK) {
            argNames.push_back(getToken().val);
            if (getToken().tok != lexer::IDENT_TK) {
                log_fatal("fatal");
                std::exit(1);
            }
            passToken();//pass arg name
            if (getToken().tok == lexer::RPAR_TK) {
                break;
            }
            if (getToken().tok != lexer::COMMA_TK) {
                log_fatal("fatal");
                std::exit(1);
            }
            passToken();//pass ,
        }
        if (getToken().tok != lexer::RPAR_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        passToken();//pass )
        minilog::log_info("parsed func decl");
        return std::make_unique<PrototypeAST>(fnName, argNames);
    }
    
    std::unique_ptr<FunctionAST> parseFuncDef() {
        if (getToken().tok != lexer::FN_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        passToken();//pass fn
        auto signature = parseFuncDecl();
        if (getToken().tok != lexer::LBRACE_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        passToken();//pass {
        auto def = parseExpression();
        if (getToken().tok != lexer::RBRACE_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        passToken();//pass }
        minilog::log_info("parsed func def");
        return std::make_unique<FunctionAST>(std::move(signature), std::move(def));
    }
    
    int Interpret(){
        while(getToken().tok!=lexer::EOF_TK){
            if (getToken().tok == lexer::FN_TK) {
                InterpretFuncDef();
            } else if (getToken().tok == lexer::EXTERN_TK) {
                InterpretExtern();
            } else if (getToken().tok == lexer::SEMICON_TK) {
                passToken();
            } else {
                InterpretTopLevelExpr();
            }
        }
        return 0;
    }
    
}