/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/AST.h"

namespace fx9next {

std::unique_ptr<Expr>
Expr::make(ExprKind kind)
{
    std::unique_ptr<Expr> expr(new Expr());
    expr->kind = kind;
    expr->floatValue = 0;
    expr->intValue = 0;
    expr->boolValue = false;
    return expr;
}

} /* namespace fx9next */
