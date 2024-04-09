#include <iostream>
#include "lexer/lexer.h"
#include <map>
#include "parser/parser.h"
std::vector<lexer::Token> tokens;
size_t tokIndex = 0;


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

std::function<void()> passToken;
std::function<lexer::Token()>getToken;

int main(int argc, char **argv) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    parser::TheJIT = DustJIT::Create();
    parser::InitModuleAndManagers();
    if(argc>1){
        // file mode
        getToken=[&]{
            if(tokIndex<tokens.size()){
                return tokens[tokIndex];
            }else{
                return lexer::Token{lexer::EOF_TK,""};
            }
        };
        passToken=[&]{
            tokIndex++;
        };

        std::ifstream source{argv[1]};
        tokens = lexer::lexFile(source);
        for (auto &tk: tokens) {
            std::cout << lexer::to_string(tk.tok) << ' ';
            if (tk.tok == lexer::IDENT_TK || tk.tok == lexer::STR_TK) {
                std::cout << tk.val << std::endl;
            }
        }

        parser::MainLoop();
    }else{
        // interactive mode
        getToken=[&]{
            if(tokIndex<tokens.size()){
                return tokens[tokIndex];
            }else{
                std::string line;
                std::getline(std::cin,line);
                tokens=lexer::lexLine(line);
                tokIndex=0;
                return getToken();
            }
        };
        passToken=[&]{
            tokIndex++;
        };


        parser::MainLoop();
    }

    

    return 0;
}
