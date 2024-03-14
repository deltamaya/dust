//
// Created by delta on 12/03/2024.
//

#ifndef DUST_LEXER_H
#define DUST_LEXER_H

#include <vector>
#include<fstream>

namespace lexer{
    #define _FOR_EACH(f) \
f(EOF_TK)           \
    f(FN_TK)\
    f(IDENT_TK)\
    f(NUM_TK)\
    f(LPAR_TK)\
    f(RPAR_TK)\
    f(LBRACE_TK)\
    f(RBRACE_TK)\
    f(LBRACKET_TK)\
    f(RBRACKET_TK)\
    f(COLON_TK)\
    f(IF_TK)\
    f(ELSE_TK)\
    f(ADD_TK)\
    f(SUB_TK)\
    f(MUL_TK)\
    f(DIV_TK)\
    f(ADDEQ_TK)\
    f(SUBEQ_TK)\
    f(MULEQ_TK)\
    f(DIVEQ_TK)\
    f(EQ_TK)\
    f(GREATEEQ_TK)\
    f(LESSEQ_TK)\
    f(GREATER_TK)\
    f(LESS_TK)\
    f(NOTEQ_TK)\
    f(DQUOTE_TK)\
    f(SQUOTE_TK)\
    f(NOT_TK)\
    f(DOT_TK)\
    f(COMMA_TK)\
    f(NUMLIT_TK)        \
f(SEMICON_TK)            \
    f(STR_TK)            \
    f(EXTERN_TK)
    
    
    
    enum TokenId {
#define _Function(name) name,
        _FOR_EACH(_Function)
#undef _Function
    };
    
    inline std::string to_string(TokenId id) {
        #define _Function(name) case name: \
    return #name;
        switch (id) {
            _FOR_EACH(_Function)
            default:
                return "unknown";
        }
        #undef _Function
    }
    
    struct Token {
        TokenId tok;
        std::string val;
    };
    
    std::vector<Token> lex(std::ifstream &source);
}
#endif //DUST_LEXER_H
