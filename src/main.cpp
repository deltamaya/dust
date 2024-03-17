#include <iostream>
#include "lexer/lexer.h"
#include <map>
#include "parser/parser.h"
#include "emitter/emitter.h"
std::vector<lexer::Token> tokens;
size_t tokIndex = 0;
lexer::Token curTok;

std::map<lexer::TokenId, int> BinOpPrecedence{
        {lexer::ADD_TK, 10},
        {lexer::SUB_TK, 10},
        {lexer::MUL_TK, 20},
        {lexer::DIV_TK, 20},
};
std::function<void()> getNextToken;


int main(int argc, char **argv) {


    if(argc>1){
        // compile mode
        getNextToken=[&]() {
            tokIndex++;
            if (tokIndex >= tokens.size()) {
                curTok = {lexer::EOF_TK, ""};
            } else {
                curTok = tokens[tokIndex];
            }
        };
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        std::ifstream source{argv[1]};
        tokens = lexer::lexFile(source);
        curTok = tokens[0];
        for (auto &tk: tokens) {
            std::cout << lexer::to_string(tk.tok) << ' ';
            if (tk.tok == lexer::IDENT_TK || tk.tok == lexer::STR_TK) {
                std::cout << tk.val << std::endl;
            }
        }
        parser::TheJIT = DustJIT::Create();
        parser::InitModuleAndManagers();
        parser::MainLoop();
        std::string outputName;
        if(argc>2){
             outputName=argv[2];
        }else{
            outputName="a.out";
        }
        emitter::Emit(outputName);

    }else{
        // interpret mode
        getNextToken=[&]() {
            static std::string line{};
            static int index=0;
            if(std::cin.eof()){
                curTok={lexer::EOF_TK,""};
                return;
            }
            if(index<tokens.size()){
                curTok=tokens[index++];
            }else{
                std::getline(std::cin,line);
                if(std::cin.eof()){
                    curTok={lexer::EOF_TK,""};
                    return;
                }
                tokens=lexer::lexLine(line);
                index=0;
                getNextToken();
            }
        };
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        parser::TheJIT = DustJIT::Create();
        parser::InitModuleAndManagers();
        getNextToken();
        parser::Interpret();
    }

    

    return 0;
}
