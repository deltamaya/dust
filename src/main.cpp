#include <iostream>
#include "lexer/lexer.h"
#include <map>
#include "parser/parser.h"
using namespace dust;

int main(int argc, char **argv) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    dust::parser::TheJIT = DustJIT::Create();
    dust::parser::InitModuleAndManagers();
    if(argc>1){
        std::ifstream source{argv[1]};
        lexer::tokens = lexer::lexFile(source);
        for (auto &tk:  lexer::tokens) {
            std::cout << lexer::to_string(tk.tok) << ' ';
            if (tk.tok == lexer::IDENT_TK || tk.tok == lexer::STR_TK) {
                std::cout << tk.val << std::endl;
            }
        }
        parser::SetParseMode(parser::File);
        parser::MainLoop();
    }else{
        parser::SetParseMode(parser::Interactive);
        parser::MainLoop();
    }

    

    return 0;
}
