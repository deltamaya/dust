//
// Created by delta on 12/03/2024.
//


#include "parser/parser.h"


namespace parser{
    using namespace minilog;
    using namespace ast;
    bool isBinOperator(const lexer::Token &tk) {
        return BinOpPrecedence.contains(tk.tok);
    }
    
    int getTokPrecedence() {
        if (isBinOperator(getToken()))
            return BinOpPrecedence[getToken().tok];
        else return -1;
    }
    
    uexpr MainLoop() {
        while (getToken().tok != lexer::EOF_TK) {
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
        }else if (getToken().tok == lexer::IF_TK) {
            return parseIfExpr();
        }else if (getToken().tok == lexer::FOR_TK) {
            return parseForExpr();
        }else if (getToken().tok == lexer::VAR_TK) {
            return parseVarExpr();
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
    
    std::unique_ptr<ReturnStmtAST> parseReturnStmt(){
        passToken();//pass return
        auto retStmt= std::make_unique<ReturnStmtAST>(std::move(parseExpression()));
        if(getToken().tok!=lexer::SEMICON_TK){
            std::exit(3);
        }
        passToken();//pass ;
        return retStmt;
    }
    std::unique_ptr<RegularStmtAST> parseRegularStmt(){
        auto  reguStmt=std::make_unique<RegularStmtAST>(std::move(parseExpression()));
        if(getToken().tok!=lexer::SEMICON_TK){
            std::exit(3);
        }
        passToken();//pass ;
        return reguStmt;
    }
    std::unique_ptr<StmtAST> parseStatement(){
        if(getToken().tok==lexer::RET_TK){
            return parseReturnStmt();
        }
        return parseRegularStmt();
    }
    
    std::vector<std::unique_ptr<StmtAST>> parseCodeBlock(){
        std::vector<std::unique_ptr<StmtAST>>stmts;
        while(getToken().tok!=lexer::RBRACE_TK){
            stmts.emplace_back(parseStatement());
        }
        return stmts;
    }
    
    std::unique_ptr<FunctionAST> parseTopLevelExpr() {
        if (auto e = parseRegularStmt()) {
            auto proto = std::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
            std::vector<std::unique_ptr<StmtAST>>stmt;
            stmt.emplace_back(std::make_unique<ReturnStmtAST>(std::move(e)));
            return std::make_unique<FunctionAST>(std::move(proto), std::move(stmt));
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
        auto def = parseCodeBlock();
        if (getToken().tok != lexer::RBRACE_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        passToken();//pass }
        minilog::log_info("parsed func def");
        return std::make_unique<FunctionAST>(std::move(signature), std::move(def));
    }
    
    std::unique_ptr<ExprAST> parseIfExpr(){
        passToken();//pass if
        auto Cond=parseExpression();
        if(!Cond)return nullptr;
        passToken();//pass {
        auto Then=parseExpression();
        passToken();//pass }
        if(getToken().tok!=lexer::ELSE_TK){
            minilog::log_error("expect else");
            std::exit(1);
        }
        passToken();//pass else
        passToken();//pass {
        auto Else=parseExpression();
        passToken();//pass }
        return std::make_unique<IfExprAST>(std::move(Cond),std::move(Then),std::move(Else));
    }
    
    std::unique_ptr<ExprAST>parseForExpr(){
        passToken();//pass for
        std::string varName=getToken().val;
        passToken();
        passToken();//pass =
        auto Start=parseExpression();
        passToken();//pass ;
        auto End=parseExpression();
        uexpr Step;
        if(getToken().tok==lexer::SEMICON_TK){
            passToken();//pass ;
            Step=parseExpression();
            if(!Step)return nullptr;
        }
        passToken();//pass {
        auto Body=parseExpression();
        passToken();//pass }
        return std::make_unique<ForExprAST>(varName,std::move(Start),std::move(End),std::move(Step),std::move(Body));
    }
    
    std::unique_ptr<ExprAST> parseVarExpr(){
        passToken();//pass var
        std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
        
        // At least one variable name is required.
        if (getToken().tok != lexer::IDENT_TK){
            minilog::log_error("expect identifier");
            return nullptr;
        }
        while (true) {
            std::string Name = getToken().val;
            passToken();  // pass identifier.
            
            // Read the optional initializer.
            std::unique_ptr<ExprAST> Init;
            if (getToken().tok == lexer::ASSIGN_TK) {
                passToken(); // eat the '='.
                
                Init = parseExpression();
                if (!Init) return nullptr;
            }
            
            VarNames.emplace_back(Name, std::move(Init));
            
            // End of var list, exit loop.
            if (getToken().tok != lexer::COMMA_TK) break;
            passToken(); // eat the ','.
            
            if (getToken().tok != lexer::IDENT_TK){
                minilog::log_error("expect identifier");
                return nullptr;
            }
            
        }

        if (getToken().tok != lexer::SEMICON_TK)
            return nullptr;
        passToken();  // eat ';'.
        
        auto Body = parseExpression();
        if (!Body)
            return nullptr;
        
        return std::make_unique<VarExprAST>(std::move(VarNames),
                                            std::move(Body));
    }
    
    
}