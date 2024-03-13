#include <iostream>
#include "lexer/lexer.h"

std::vector<lexer::Token> tokens;
lexer::Token curTok;
size_t tokIndex=0;
void getNextToken(){
    tokIndex++;
    curTok=tokens[tokIndex];
}
int main(int argc,char**argv) {
    if(argc<2){
        std::cout<<"you need to specify source file\n";
        std::exit(0);
    }else{
        std::ifstream source{argv[1]};
        tokens=lexer::lex(source);
        for(auto&tk:tokens){
            std::cout<<lexer::to_string(tk.tok)<<' ';
            if(tk.tok==lexer::IDENT_TK||tk.tok==lexer::STR_TK){
                std::cout<<tk.val<<std::endl;
            }
        }
        curTok=tokens.front();
    }
    return 0;
}
