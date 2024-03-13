#include <iostream>
#include "lexer/lexer.h"
#include <map>
#include "parser/parser.h"
std::vector<lexer::Token> tokens;
size_t tokIndex=0;
lexer::Token curTok;

std::map<lexer::TokenId,int>BinOpPrecedence{
        {lexer::ADD_TK,10},
        {lexer::SUB_TK,10},
        {lexer::MUL_TK,20},
        {lexer::DIV_TK,20},
};
void getNextToken(){
    tokIndex++;
    if(tokIndex>=tokens.size()){
        curTok= {lexer::EOF_TK,""};
    }
    else{
        curTok=tokens[tokIndex];
    }
}
int main(int argc,char**argv) {
    if(argc<2){
        std::cout<<"you need to specify source file\n";
        std::exit(0);
    }else{
        std::ifstream source{argv[1]};
        tokens=lexer::lex(source);
        curTok=tokens[0];
        for(auto&tk:tokens){
            std::cout<<lexer::to_string(tk.tok)<<' ';
            if(tk.tok==lexer::IDENT_TK||tk.tok==lexer::STR_TK){
                std::cout<<tk.val<<std::endl;
            }
        }
        parser::ParseAll();
    }
    return 0;
}
