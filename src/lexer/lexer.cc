//
// Created by delta on 12/03/2024.
//
#include "lexer/lexer.h"
#include "minilog.h"
#include <cctype>

namespace lexer{
    bool isBound(char ch){
        return ch=='('||ch==')'||ch=='['||ch==']'||ch=='{'||ch=='}'||ch==','||ch=='.'||ch==':';
    }
    bool isOperator(char id){
        return id=='+'||id=='-'||id=='*'||id=='/'||id=='='||id=='>'||id=='<';
    }
    bool isIdent(const std::string& str){
        return !std::isdigit(str[0]);
    }
    bool isInteger(const std::string&str){
        for(auto ch:str){
            if(!std::isdigit(ch)){
                return false;
            }
        }
        return true;
    }
    Token lexString(std::string const&str){
        if(str=="fn"){
            return {FN_TK,""};
        }else if(str=="i32"){
            return {I32_TK,""};
        }else if(str=="if"){
            return {IF_TK, ""};
        }else if(str=="else"){
            return {ELSE_TK,""};
        }else if(str=="("){
            return{LPAR_TK,""};
        }else if(str==")"){
            return {RPAR_TK,""};
        }else if(str=="["){
            return {LBRACKET_TK,""};
        }else if(str=="]"){
            return {RBRACKET_TK,""};
        }else if(str=="{"){
            return {LBRACE_TK,""};
        }else if(str=="}"){
            return {RBRACE_TK,""};
        }else if(str==":"){
            return {COLON_TK,""};
        }else if(str=="+"){
            return {ADD_TK,""};
        }else if(str=="+="){
            return {ADDEQ_TK,""};
        }else if(str=="-"){
            return {SUB_TK,""};
        }else if(str=="-="){
            return {SUBEQ_TK,""};
        }else if(str=="*"){
            return {MUL_TK,""};
        }else if(str=="*="){
            return {MULEQ_TK,""};
        }else if(str=="/"){
            return {DIV_TK,""};
        }else if(str=="/="){
            return {DIVEQ_TK,""};
        }else if(str=="\'"){
            return {SQUOTE_TK,""};
        }else if(str=="\""){
            return {DQUOTE_TK,""};
        }else if(str=="<"){
            return {LESS_TK,""};
        }else if(str=="<="){
            return {LESSEQ_TK,""};
        }else if(str=="=="){
            return {EQ_TK,""};
        }else if(str==">="){
            return {GREATEEQ_TK,""};
        }else if(str==">"){
            return {GREATER_TK,""};
        }else if(str=="!="){
            return {NOTEQ_TK,""};
        }else if(str=="!"){
            return {NOT_TK,""};
        }else if(isIdent(str)){
            return {IDENT_TK,str};
        }else if(isInteger(str)){
            return {INTEGER_TK,str};
        }
        else{
            minilog::log_fatal("can not parse: {}",str);
            std::exit(-1);
        }
    }
    std::vector<Token> lex(std::ifstream& source){
        std::vector<Token>ret;
        char ch;
        std::string buf;
        while((ch=source.get())!=-1){
            if(!std::isalnum(ch)){
                if(!buf.empty()){
                    ret.push_back(lexString(buf));
                    buf.clear();
                }
                if(std::isspace(ch)){
                    continue;
                }
                buf.push_back(ch);
                // check if it is operators, such as +=, <=, of just +, -, >
                if(isOperator(ch)||ch=='!'){
                    char next=source.peek();
                    if(next=='='){
                        source.get();
                        buf.push_back(next);
                    }
                    ret.push_back(lexString(buf));
                    buf.clear();
                }else if(isBound(ch)){
                    ret.push_back(lexString(buf));
                    buf.clear();
                }
            }else{
                buf+=ch;
            }
        }
        if(!buf.empty()){
            ret.push_back(lexString(buf));
        }
        ret.push_back({EOF_TK,""});
        return ret;
    }
}