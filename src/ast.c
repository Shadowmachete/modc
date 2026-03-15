#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "modc_types.h"
#include "utils.h"

Ast *ast_create(void) {
  // TODO: replace with arena
  Ast *ast = malloc(sizeof(Ast));

  if (!ast)
    error(0, "malloc failed creating ast");

  memset(ast, 0, sizeof(Ast));

  return ast;
}

void ast_free(Ast *ast) { free(ast); }

Ast *ast_int64(int64_t val) {
  Ast *ast = ast_create();

  ast->variant = AST_INTEGER;
  ast->type = int_64_type;
  ast->number.value = val;

  return ast;
}

Ast *ast_float64(double val) {
  Ast *ast = ast_create();

  ast->variant = AST_FLOAT;
  ast->type = float_64_type;
  ast->fvalue.value = val;

  return ast;
}

Ast *ast_char(int64_t val) {
  Ast *ast = ast_create();

  ast->variant = AST_INTEGER;
  ast->type = char_8_type;
  ast->number.value = val;

  return ast;
}

Ast *ast_string(char *str, size_t len) {
  Ast *ast = ast_create();

  ast->variant = AST_STRING;
  ast->string.data = strndup(str, len);
  ast->string.length = len;

  /* ast->type = astMakeArrayType(); */
  return ast;
}

// TODO: add functions for the rest of the tokens
// At least for a minimal program at first (e.g. if statements, var declaration,
// main function and return)
