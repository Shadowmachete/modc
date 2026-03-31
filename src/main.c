#include <stdio.h>
#include <stdlib.h>

#include "arena.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"

#define ARENA_CAPACITY (1024 * 64)

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs("usage: main file.modc\n", stderr);
    exit(1);
  }

  char *source = readin(argv[1]);
  printf("Source: \n%s\n", source);

  global_arena_init(ARENA_CAPACITY);
  ast_memory_init();

  Lexer lexer;
  Parser *parser = parser_create(&lexer);

  parse(parser, source); // produce AST

  ast_print(parser->ast, 0);

  free(source);
  parser_free(parser);
  global_arena_release();
  ast_memory_release();
}
