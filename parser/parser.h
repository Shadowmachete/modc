#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "../lexer/lexer.h"
#include "../symtab/symtab.h"

typedef struct {
  // AST syntax_tree;
  Lexer *lexer;
  SymTable *symtab;

  int pass_no;

  // this can be improved in the future
  TokenType current_type;
  bool type_is_pointer;
  uint8_t current_pointer_depth;
} Parser;

Parser *parser_create(Lexer *l);

void parse(Parser *parser, const char *source);
void parse_token(Parser *parser, Token t);

#endif
