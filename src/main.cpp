#include <iostream>
#include "lexer/lexer.h"
#include <map>
#include "parser/parser.h"
using namespace dust;

int main(int argc, char **argv) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    parser::TheJIT = DustJIT::Create();
    parser::InitModuleAndManagers();
    if(argc>1){
        std::ifstream source{argv[1]};
        lexer::tokens = lexer::lexFile(source);
        parser::SetParseMode(parser::File);
    }else{
        parser::SetParseMode(parser::Interactive);
    }
    parser::MainLoop();
    

    return 0;
}
