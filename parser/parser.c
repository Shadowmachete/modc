#include <stdlib.h>

#include "../symtab/symtab.h"
#include "../utils.h"
#include "parser.h"

Parser *parser_create(Lexer *l) {
  Parser *parser = malloc(sizeof(Parser));

  if (!parser)
    error(0, "malloc failed creating parser");

  parser->pass_no = 0;
  parser->symtab = symtab_create();
  parser->current_type = TOKEN_UNKNOWN;
  parser->type_is_pointer = false;
  parser->current_pointer_depth = 0;
  parser->lexer = l;

  return parser;
}

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
  Symbol *sym;
  Token next_tok = lex_peek(parser->lexer);

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
    parser->current_type = t.type;
    if (next_tok.type != TOKEN_IDENTIFIER && next_tok.type != TOKEN_ASTERIX &&
        next_tok.type != TOKEN_RPAREN) {
      error(next_tok.line, "symbol following a type annotation must be '*', "
                           "')' or an identifier");
    }

    if (next_tok.type == TOKEN_ASTERIX) {
      parser->type_is_pointer = true;
    } else {
      parser->type_is_pointer = false;
    }

    while (next_tok.type == TOKEN_ASTERIX) {
      next_tok = lex_next(parser->lexer);
      parser->current_pointer_depth++;
    }

    break;
  case TOKEN_IDENTIFIER:
    if (next_tok.type != TOKEN_COLON && parser->current_type == TOKEN_UNKNOWN &&
        !symtab_lookup(parser->symtab, t.start, t.length))
      error(t.line, "identifier '%.*s' used before declaration", (int)t.length,
            t.start);

    // 1st pass: store symbols
    if (parser->pass_no == 1 &&
        !symtab_lookup(parser->symtab, t.start, t.length)) {
      sym = symtab_insert(parser->symtab, t.start, t.length);
      sym->type = parser->current_type;
      if (parser->type_is_pointer) {
        sym->is_pointer = true;
        sym->pointer_depth = parser->current_pointer_depth;
      } else {
        sym->is_pointer = false;
      }
      switch (next_tok.type) {
      case TOKEN_LPAREN:
        sym->variant = FUNCTION;
        break;
      case TOKEN_COLON:
        sym->variant = LABEL;
        sym->type = TOKEN_UNKNOWN;
        break;
      case TOKEN_COMMA:
      case TOKEN_RPAREN:
        sym->variant = PARAMETER;
        break;
      default:
        sym->variant = VARIABLE;
        break;
      }
    }

    break;
  case TOKEN_ASTERIX:
    // 1st pass: store symbols
    if (parser->pass_no == 1 && parser->type_is_pointer) {
      break;
    }

    break;
  default:
    parser->current_type = TOKEN_UNKNOWN;
    parser->type_is_pointer = false;
    parser->current_pointer_depth = 0;

    break;
  }
}
