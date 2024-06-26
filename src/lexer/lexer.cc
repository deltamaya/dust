//
// Created by delta on 12/03/2024.
//
#include "lexer/lexer.h"
#include "utils/minilog.h"
#include <cctype>

namespace dust::lexer{
    std::vector<Token> tokens;
    size_t tokIndex = 0;
    bool isBound(char ch) {
        return ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}' || ch == ',' || ch == ':' ||
               ch == ';';
    }
    
    bool isOperator(char id) {
        return id == '+' || id == '-' || id == '*' || id == '/' || id == '=' || id == '>' || id == '<';
    }
    
    bool isIdent(const std::string &str) {
        return !std::isdigit(str[0]) && std::ranges::all_of(str, [](char ch) { return std::isalnum(ch); });
    }
    
    bool isNumber(const std::string &str) {
        bool hasDot = false;
        for (auto ch: str) {
            if (!std::isdigit(ch)) {
                if (ch == '.') {
                    if (hasDot) {
                        return false;
                    }
                    hasDot = true;
                } else {
                    return false;
                }
            }
        }
        return true;
    }
    
    bool isString(const std::string &str) {
        return str.front() == '\"' && str.back() == '\"';
    }
    
    Token lexString(std::string const &str) {
        if (str == "fn") {
            return {FN_TK, ""};
        } else if (str == "num") {
            return {NUM_TK, ""};
        } else if (str == "if") {
            return {IF_TK, ""};
        } else if (str == "else") {
            return {ELSE_TK, ""};
        } else if (str == "extern") {
            return {EXTERN_TK, ""};
        } else if (str == "for") {
            return {FOR_TK, ""};
        }else if (str == "return") {
            return {RET_TK, ""};
        }else if (str == "var") {
            return {VAR_TK, ""};
        }else if (str == "str") {
            return {STR_TK, ""};
        } else if (str == "(") {
            return {LPAR_TK, ""};
        } else if (str == ")") {
            return {RPAR_TK, ""};
        } else if (str == "[") {
            return {LBRACKET_TK, ""};
        } else if (str == "]") {
            return {RBRACKET_TK, ""};
        } else if (str == "{") {
            return {LBRACE_TK, ""};
        } else if (str == "}") {
            return {RBRACE_TK, ""};
        } else if (str == ":") {
            return {COLON_TK, ""};
        } else if (str == ",") {
            return {COMMA_TK, ""};
        } else if (str == ";") {
            return {SEMICON_TK, ""};
        } else if (str == "+") {
            return {ADD_TK, ""};
        } else if (str == "+=") {
            return {ADDEQ_TK, ""};
        } else if (str == "-") {
            return {SUB_TK, ""};
        } else if (str == "-=") {
            return {SUBEQ_TK, ""};
        } else if (str == "*") {
            return {MUL_TK, ""};
        } else if (str == "*=") {
            return {MULEQ_TK, ""};
        } else if (str == "/") {
            return {DIV_TK, ""};
        } else if (str == "/=") {
            return {DIVEQ_TK, ""};
        } else if (str == "\'") {
            return {SQUOTE_TK, ""};
        } else if (str == "\"") {
            return {DQUOTE_TK, ""};
        } else if (str == "<") {
            return {LESS_TK, ""};
        } else if (str == "<=") {
            return {LESSEQ_TK, ""};
        } else if (str == "==") {
            return {EQ_TK, ""};
        } else if (str == ">=") {
            return {GREATEEQ_TK, ""};
        } else if (str == ">") {
            return {GREATER_TK, ""};
        } else if (str == "!=") {
            return {NOTEQ_TK, ""};
        } else if (str == "!") {
            return {NOT_TK, ""};
        }else if (str == "=") {
            return {ASSIGN_TK, ""};
        } else if (isString(str)) {
            std::string literal = str.substr(1, str.size() - 2);
//            minilog::log_debug("lexer phase: {}",literal);
            return {STRLIT_TK, literal};
        } else if (isIdent(str)) {
            return {IDENT_TK, str};
        } else if (isNumber(str)) {
            return {NUMLIT_TK, str};
        } else {
            minilog::log_fatal("can not parse: {}", str);
            std::exit(-1);
        }
    }
    
    template<typename InputStream>requires requires(InputStream i){i.get();i.peek();}
    std::vector<Token> lexGeneric(InputStream&source){
        std::vector<Token> ret;
        char ch;
        std::string buf;
        auto lexBuf = [&] {
            ret.push_back(lexString(buf));
            buf.clear();
        };
        while ((ch = source.get()) != -1) {
            if (!std::isalnum(ch)) {
                if (!buf.empty()) {
                    lexBuf();
                }
                if(ch=='#'){
                    // comments
                    while((ch = source.get()) != -1){
                        if(ch=='\n'||ch=='\r'){
                            break;
                        }
                    }
                    continue;
                }
                if (std::isspace(ch)) {
                    continue;
                }
                //lex string literals
                buf += ch;
                if (ch == '\"') {
                    while ((ch = source.get()) != -1) {
                        buf += ch;
                        if (ch == '\"') {
                            lexBuf();
                            break;
                        }
                    }
                    continue;
                }
                // check if it is operators, such as +=, <=, of just +, -, >
                if (isOperator(ch) || ch == '!') {
                    char next = source.peek();
                    if (next == '=') {
                        source.get();
                        buf += next;
                    }
                    lexBuf();
                } else if (isBound(ch)) {
                    lexBuf();
                }
            } else if (std::isalpha(ch)) {
                buf += ch;
            } else {
                //lex numbers such as floats or integers
                if (buf.empty()) {
                    buf += ch;
                    bool hasdot = false;
                    while ((ch = source.peek()) != -1) {
                        if (std::isdigit(ch)) {
                            source.get();
                            buf += ch;
                        } else {
                            if (ch == '.' && !hasdot) {
                                buf += ch;
                                source.get();
                                hasdot = true;
                            } else {
                                lexBuf();
                                break;
                            }
                        }
                    }
                }else{
                    buf += ch;
                }
            }
        }
        
        if (!buf.empty()) {
            ret.push_back(lexString(buf));
        }
        return ret;
    }
    
    std::vector<Token>lexLine(const std::string &line){
        std::stringstream source{line};
        return lexGeneric(source);
    }
    
    std::vector<Token> lexFile(std::ifstream &source) {
        return lexGeneric(source);
    }
    


}