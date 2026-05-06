#ifndef SEMA_H
#define SEMA_H

#include "ast.h"
#include "modc_types.h"
#include "scope.h"

void sema_check(Ast *ast, Scope *current_scope, ModCType *return_type);

#endif // !SEMA_H
