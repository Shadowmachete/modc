#include <stdio.h>
#include <stdlib.h>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "symtab/symtab.h"
#include "utils.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs("usage: main file.modc\n", stderr);
    exit(1);
  }

  char *source = readin(argv[1]);

  printf("Source: \n%s\n", source);

  Lexer lexer;

  Parser *parser = parser_create(&lexer);

  parse(parser, source);

  symtab_print(parser->symtab);

  free(source);
}
