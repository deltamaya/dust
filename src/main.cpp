#include <iostream>
#include "lexer/lexer.h"
int main(int argc,char**argv) {
    if(argc<2){
        std::cout<<"you need to specify source file\n";
        std::exit(0);
    }else{
        std::ifstream source{argv[1]};
        auto tokens=lexer::lex(source);
        for(auto&tk:tokens){
            std::cout<<lexer::to_string(tk.tok)<<' ';
        }
    }
    return 0;
}
