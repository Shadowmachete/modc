#include <stdio.h>
#include <stdlib.h>

#include "arena.h"
#include "ast.h"
#include "ir.h"
#include "lexer.h"
#include "modc_types.h"
#include "parser.h"
#include "scope.h"
#include "sema.h"
#include "utils.h"

#define ARENA_CAPACITY (1024 * 64)

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs("usage: modcc file.modc\n", stderr);
    exit(1);
  }

  File f = readin(argv[1]);
  printf("Source: \n%s\n", f.src);

  global_arena_init(ARENA_CAPACITY);
  modctype_memory_init();
  ast_memory_init();
  scope_memory_init();
  ir_memory_init();

  Lexer lexer;
  Parser *parser = parser_create(&lexer);

  parse(parser, &f);

  check_errors();

  /* ast_print(parser->ast, 0); */

  Scope *global_scope = make_global_scope(&f);

  parser->ast->block.scope = global_scope;

  populate_scopes(parser->ast, global_scope);

  sema_check(parser->ast, global_scope, NULL);

  check_errors();

  IrProgram *program = ir_gen(parser->ast);
  // TODO: implement CFG, SSA and use memory read-modify-write operations
  // instead of separate instructions
  ir_display(program);

  check_errors();

  // TODO: IR optimisation

  // TODO: Code gen

  parser_free(parser);

  free((void *)f.src);

  ir_memory_release();
  scope_memory_release(global_scope);
  ast_memory_release();
  modctype_memory_release();
  global_arena_release();
}
