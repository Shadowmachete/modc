#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stdint.h>

#include "ast.h"
#include "lexer.h"

typedef struct {
  Ast *ast;
  Lexer *lexer;

  int pass_no;
} Parser;

Parser *parser_create(Lexer *l);
void parser_free(Parser *parser);

void parse(Parser *parser, const char *source);
void parse_token(Parser *parser, Token t);

#endif // !PARSER_H
