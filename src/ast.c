#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "ast.h"
#include "modc_types.h"
#include "str.h"
#include "utils.h"

static Arena ast_arena;
static int ast_arena_init = 0;

void ast_memory_init(void) {
  if (!ast_arena_init) {
    arena_init(&ast_arena, sizeof(Ast) * 512);
    ast_arena_init = 1;
  }
}

void ast_memory_release(void) {
  if (ast_arena_init) {
    ast_arena_init = 0;
    arena_clear(&ast_arena);
  }
}

static Ast *ast_alloc(void) {
  return (Ast *)arena_alloc(&ast_arena, sizeof(Ast));
}

Ast *ast_create(void) {
  Ast *ast = ast_alloc();

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
  ast->string.value = str_dup_raw(str, len);

  /* ast->type = astMakeArrayType(); */
  return ast;
}

// TODO: add functions for the rest of the tokens
// At least for a minimal program at first (e.g. if statements, var declaration,
// main function and return)
