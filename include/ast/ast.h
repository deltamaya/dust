//
// Created by delta on 12/03/2024.
//

#ifndef DUST_AST_H
#define DUST_AST_H

#include <string>
#include <memory>
#include <utility>
#include <vector>
#include "lexer/lexer.h"
class ExprAST {
public:
    virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST {
    double val;
public:
   explicit NumberExprAST(double v) : val(v) {}
};

class StringExprAST : public ExprAST {
    std::string val;
public:
    explicit StringExprAST(std::string v) : val(std::move(v)) {}
};


class VariableExprAST : public ExprAST {
    std::string name;
public:
   explicit VariableExprAST(std::string name):name(std::move(name)){}
};

class BinaryExprAST : public ExprAST {
    lexer::Token op;
    std::unique_ptr<ExprAST> lhs, rhs;
public:
    BinaryExprAST(lexer::Token op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs) :
            op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
};

class CallExprAST : public ExprAST {
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;
public:
    CallExprAST(std::string Callee,
                std::vector<std::unique_ptr<ExprAST>> Args)
            : callee(std::move(Callee)), args(std::move(Args)) {}
};

class PrototypeAST:public ExprAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(std::string Name, std::vector<std::string> Args)
            : Name(std::move(Name)), Args(std::move(Args)) {}
    
    [[nodiscard]] const std::string &getName() const { return Name; }
};

class FunctionAST :public ExprAST{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {}
};



#endif //DUST_AST_H
