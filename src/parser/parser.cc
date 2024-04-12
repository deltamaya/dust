//
// Created by delta on 12/03/2024.
//


#include "parser/parser.h"
#include "ast/func.h"

namespace dust::parser{
    using namespace minilog;
    using namespace ast;
    
    std::map<lexer::TokenId, int> BinOpPrecedence{
            {lexer::ADD_TK, 10},
            {lexer::SUB_TK, 10},
            {lexer::MUL_TK, 20},
            {lexer::DIV_TK, 20},
            {lexer::LESS_TK,5},
            {lexer::GREATER_TK,5},
            {lexer::LESSEQ_TK,5},
            {lexer::GREATEEQ_TK,5},
            {lexer::EQ_TK,5},
            {lexer::NOTEQ_TK,5},
            {lexer::ASSIGN_TK,2},
        
    };
    std::function<void()> PassToken;
    std::function<lexer::Token()>GetToken;
    

    void SetParseMode(ParseMode m){
        switch (m) {
            case Interactive:
                // interactive mode
                GetToken=[&]{
                    if( lexer::tokIndex< lexer::tokens.size()){
                        return  lexer::tokens[ lexer::tokIndex];
                    }else{
                        std::string line;
                        std::getline(std::cin,line);
                        lexer::tokens=lexer::lexLine(line);
                        lexer::tokIndex=0;
                        return  parser::GetToken();
                    }
                };
                PassToken=[&]{
                    lexer::tokIndex++;
                };
                break;
            case File:
                // file mode
                GetToken=[&]{
                    if(lexer::tokIndex<lexer::tokens.size()){
                        return lexer::tokens[lexer::tokIndex];
                    }else{
                        return lexer::Token{lexer::EOF_TK,""};
                    }
                };
                PassToken=[&]{
                    lexer::tokIndex++;
                };
                break;
            default:
                minilog::log_error("Invalid Parse Mode");
                break;
        }
    }
    
    bool isBinOperator(const lexer::Token &tk) {
        return BinOpPrecedence.contains(tk.tok);
    }
    
    int getTokPrecedence() {
        if (isBinOperator(GetToken()))
            return BinOpPrecedence[GetToken().tok];
        else return -1;
    }
    
    uexpr MainLoop() {
        while (GetToken().tok != lexer::EOF_TK) {
            if (GetToken().tok == lexer::FN_TK) {
                InterpretFuncDef();
            } else if (GetToken().tok == lexer::EXTERN_TK) {
                InterpretExtern();
            } else {
                InterpretTopLevelExpr();
            }
        }
        return nullptr;
    }
    
    std::unique_ptr<PrototypeAST> parseExtern() {
        PassToken();//pass extern
        auto ret= parseFuncDecl();
        PassToken();//pass ;
        return ret;
    }
    
    uexpr parseNumberExpr() {
        auto ret = std::make_unique<NumberExprAST>(std::stod(GetToken().val));
        PassToken();
//        log_info("num literal expr");
        return ret;
    }
    
    uexpr parseIdentifierExpr() {
        std::string name = GetToken().val;
        PassToken();//pass name
        if (GetToken().tok != lexer::LPAR_TK) {
//            log_info("ident expr");
            return std::make_unique<VariableExprAST>(name);
        }
        PassToken();//pass (
        std::vector<uexpr> args;
        while (GetToken().tok != lexer::RPAR_TK) {
            if (auto Arg = parseExpression();Arg)
                args.push_back(std::move(Arg));
            else
                return nullptr;
            if (GetToken().tok != lexer::COMMA_TK) {
                if (GetToken().tok == lexer::RPAR_TK) {
                    break;
                } else {
                    minilog::log_fatal("expect , or )");
                    std::exit(1);
                }
            }
            PassToken();//pass ,
        }
        PassToken();//pass )
//        log_info("func call expr");
        return std::make_unique<CallExprAST>(name, std::move(args));
    }
    
    uexpr parseParenthesisExpr() {
        PassToken();//pass '('
        auto v = parseExpression();
        if (!v) {
            return nullptr;
        }
        if (GetToken().tok != lexer::RPAR_TK) {
            minilog::log_fatal("can not parse parenthesis expression");
            std::exit(-1);
        }
        PassToken();//pass ')'
        log_info("() expr");
        return v;
    }
    
    uexpr parseStringExpr() {
        auto v = std::make_unique<StringExprAST>(GetToken().val);
        PassToken();//pass string literal
        log_info("string expr");
        return v;
    }
    
    uexpr parsePrimary() {
        if (GetToken().tok == lexer::IDENT_TK) {
            return parseIdentifierExpr();
        } else if (GetToken().tok == lexer::NUMLIT_TK) {
            return parseNumberExpr();
        } else if (GetToken().tok == lexer::STR_TK) {
            return parseStringExpr();
        } else if (GetToken().tok == lexer::LPAR_TK) {
            return parseParenthesisExpr();
        }else if (GetToken().tok == lexer::IF_TK) {
            return parseIfExpr();
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
//            minilog::log_debug("tok {} precedence: {}", lexer::to_string(GetToken().tok), tokPrec);
            if (tokPrec < exprPrec) {
                return lhs;
            }
            auto op = GetToken();
            PassToken();//pass bin op
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
        PassToken();//pass return
        auto retStmt= std::make_unique<ReturnStmtAST>(std::move(parseExpression()));
        if(GetToken().tok != lexer::SEMICON_TK){
            std::exit(3);
        }
        PassToken();//pass ;
        return retStmt;
    }
    std::unique_ptr<RegularStmtAST> parseRegularStmt(){
        auto  reguStmt=std::make_unique<RegularStmtAST>(std::move(parseExpression()));
        if(GetToken().tok != lexer::SEMICON_TK){
            std::exit(3);
        }
        PassToken();//pass ;
        return reguStmt;
    }
    std::vector<std::unique_ptr<StmtAST>> parseCodeBlock();
    std::unique_ptr<IfStmtAST> parseIfStmt(){
        PassToken();//pass if
        auto Cond=parseExpression();
        if(!Cond)return nullptr;
        PassToken();//pass {
        auto Then=parseCodeBlock();
        PassToken();//pass }
        if(GetToken().tok == lexer::ELSE_TK){
            PassToken();//pass else
            PassToken();//pass {
            auto Else=parseCodeBlock();
            PassToken();//pass }
            minilog::log_info("parsed if statement");
            return std::make_unique<IfStmtAST>(std::move(Cond),std::move(Then),std::move(Else));
        }
        return std::make_unique<IfStmtAST>(std::move(Cond),std::move(Then),std::vector<std::unique_ptr<StmtAST>>());
       
    }
    std::unique_ptr<ForStmtAST> parseForStmt(){
        PassToken();//pass for
        std::string varName=GetToken().val;
        PassToken();
        PassToken();//pass =
        auto InitVal=parseExpression();
        PassToken();//pass ;
        auto Cond=parseExpression();
        uexpr Then;
        if(GetToken().tok == lexer::SEMICON_TK){
            PassToken();//pass ;
            Then=parseExpression();
            if(!Then)return nullptr;
        }
        PassToken();//pass {
        auto Body=parseCodeBlock();
        PassToken();//pass }
        minilog::log_info("parsed for statement");
        return std::make_unique<ForStmtAST>(varName,std::move(InitVal),std::move(Cond),std::move(Then),std::move(Body));
    }
    std::unique_ptr<VarStmtAST> parseVarStmt();
    std::unique_ptr<StmtAST> parseStatement(){
        if(GetToken().tok == lexer::RET_TK){
            return parseReturnStmt();
        }else if(GetToken().tok == lexer::IF_TK){
            return parseIfStmt();
        }else if(GetToken().tok == lexer::FOR_TK){
            return parseForStmt();
        }else if(GetToken().tok == lexer::VAR_TK){
            return parseVarStmt();
        }else if(GetToken().tok == lexer::SEMICON_TK){
            PassToken();//pass empty statement
            return std::make_unique<EmptyStmt>();
        }else{
            return parseRegularStmt();
        }
        
    }
    
    std::vector<std::unique_ptr<StmtAST>> parseCodeBlock(){
        std::vector<std::unique_ptr<StmtAST>>stmts;
        while(GetToken().tok != lexer::RBRACE_TK){
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
        
        std::string fnName = GetToken().val;
        if (GetToken().tok != lexer::IDENT_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        PassToken();//pass name
        if (GetToken().tok != lexer::LPAR_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        PassToken();//pass (
        std::vector<std::string> argNames;
        while (GetToken().tok != lexer::RPAR_TK) {
            argNames.push_back(GetToken().val);
            if (GetToken().tok != lexer::IDENT_TK) {
                log_fatal("fatal");
                std::exit(1);
            }
            PassToken();//pass arg name
            if (GetToken().tok == lexer::RPAR_TK) {
                break;
            }
            if (GetToken().tok != lexer::COMMA_TK) {
                log_fatal("fatal");
                std::exit(1);
            }
            PassToken();//pass ,
        }
        if (GetToken().tok != lexer::RPAR_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        PassToken();//pass )
//        minilog::log_info("parsed func decl");
        return std::make_unique<PrototypeAST>(fnName, argNames);
    }
    
    std::unique_ptr<FunctionAST> parseFuncDef() {
        if (GetToken().tok != lexer::FN_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        PassToken();//pass fn
        auto signature = parseFuncDecl();
        if (GetToken().tok != lexer::LBRACE_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        PassToken();//pass {
        auto def = parseCodeBlock();
        if (GetToken().tok != lexer::RBRACE_TK) {
            log_fatal("fatal");
            std::exit(1);
        }
        PassToken();//pass }
//        minilog::log_info("parsed func def");
        return std::make_unique<FunctionAST>(std::move(signature), std::move(def));
    }
    
    std::unique_ptr<ExprAST> parseIfExpr(){
        PassToken();//pass if
        auto Cond=parseExpression();
        if(!Cond)return nullptr;
        PassToken();//pass {
        auto Then=parseExpression();
        PassToken();//pass }
        if(GetToken().tok != lexer::ELSE_TK){
            minilog::log_error("expect else");
            std::exit(1);
        }
        PassToken();//pass else
        PassToken();//pass {
        auto Else=parseExpression();
        PassToken();//pass }
        return std::make_unique<IfExprAST>(std::move(Cond),std::move(Then),std::move(Else));
    }
    

    
    std::unique_ptr<VarStmtAST> parseVarStmt(){
        PassToken();//pass var
        std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
        
        // At least one variable name is required.
        if (GetToken().tok != lexer::IDENT_TK){
            minilog::log_error("expect identifier");
            return nullptr;
        }
        while (true) {
            std::string Name = GetToken().val;
            PassToken();  // pass identifier.
            
            // Read the optional initializer.
            std::unique_ptr<ExprAST> Init;
            if (GetToken().tok == lexer::ASSIGN_TK) {
                PassToken(); // eat the '='.
                
                Init = parseExpression();
                if (!Init) return nullptr;
            }
            
            VarNames.emplace_back(Name, std::move(Init));
            
            // End of var list, exit loop.
            if (GetToken().tok != lexer::COMMA_TK) break;
            PassToken(); // eat the ','.
            
            if (GetToken().tok != lexer::IDENT_TK){
                minilog::log_error("expect identifier");
                return nullptr;
            }
            
        }

        if (GetToken().tok != lexer::SEMICON_TK)
            return nullptr;
        PassToken();  // eat ';'.
        
        auto Body = parseCodeBlock();
        
        return std::make_unique<VarStmtAST>(std::move(VarNames),
                                            std::move(Body));
    }
    
    
}