#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "types.h"
#include "utils.h"

Ast *astCreate(void) {
  // TODO: replace with arena
  Ast *ast = malloc(sizeof(Ast));

  if (!ast)
    error(0, "malloc failed creating ast");

  memset(ast, 0, sizeof(Ast));

  return ast;
}

void astFree(Ast *ast) { free(ast); }

Ast *astInt64(int64_t val) {
  Ast *ast = astCreate();

  ast->variant = AST_INTEGER;
  ast->type = int_64_type;
  ast->number.value = val;

  return ast;
}

Ast *astFloat64(double val) {
  Ast *ast = astCreate();

  ast->variant = AST_FLOAT;
  ast->type = float_64_type;
  ast->fvalue.value = val;

  return ast;
}

Ast *astChar(int64_t val) {
  Ast *ast = astCreate();

  ast->variant = AST_INTEGER;
  ast->type = char_8_type;
  ast->number.value = val;

  return ast;
}

Ast *astString(char *str, size_t len) {
  Ast *ast = astCreate();

  ast->variant = AST_STRING;
  ast->string.data = strndup(str, len);
  ast->string.length = len;

  /* ast->type = astMakeArrayType(); */
  return ast;
}

// TODO: add functions for the rest of the tokens
// At least for a minimal program at first (e.g. if statements, var declaration,
// main function and return)
