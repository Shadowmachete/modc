#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "utils.h"

// TODO: change to arena

Parser *parser_create(Lexer *l) {
  Parser *parser = malloc(sizeof(Parser));

  if (!parser)
    error(0, "malloc failed creating parser");

  parser->pass_no = 0;
  parser->lexer = l;

  return parser;
}

void parser_free(Parser *parser) { free(parser); }

void parse(Parser *parser, const char *source) {
  lex_init(parser->lexer, source);

  parser->pass_no++;

  while (1) {
    Token t = lex_next(parser->lexer);
    parse_token(parser, t);

    if (t.type == TOKEN_EOF)
      break;
  }
}

void parse_token(Parser *parser, Token t) {
  Token next_tok = lex_peek(parser->lexer);

  // TODO: process each token and produce the AST

  switch (t.type) {
  case T_CHAR8:
  case T_CHAR16:
  case T_CHAR32:
  case T_UINT8:
  case T_UINT16:
  case T_UINT32:
  case T_UINT64:
  case T_INT8:
  case T_INT16:
  case T_INT32:
  case T_INT64:
  case T_FLOAT16:
  case T_FLOAT32:
  case T_FLOAT64:
  case T_BOOL:
  case T_STRUCT:
  case T_CLASS:
  case T_VOID:
    break;
  case TOKEN_IDENTIFIER:
    break;
  case TOKEN_ASTERISK:
    break;
  default:
    break;
  }

  token_print(t);
  (void)next_tok;
}
