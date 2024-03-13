//
// Created by delta on 13/03/2024.
//

#ifndef DUST_PARSER_H
#define DUST_PARSER_H
#include "ast/ast.h"
#include "lexer/lexer.h"
#include "minilog.h"
#include <map>

namespace parser{
    using uexpr = std::unique_ptr<ExprAST>;
    uexpr parseExpression();
    uexpr parseBinOpExpression(int exprPrec,uexpr lhs);
    std::unique_ptr<PrototypeAST> parseFuncDecl();
    std::unique_ptr<FunctionAST> parseFuncDef();
    uexpr ParseAll();
}
#endif //DUST_PARSER_H
