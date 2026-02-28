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

Parser *parserCreate(Lexer *l);
void parserFree(Parser *parser);

void parse(Parser *parser, const char *source);
void parseToken(Parser *parser, Token t);

#endif // !PARSER_H
