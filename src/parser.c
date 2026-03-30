#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "modc_types.h"
#include "parser.h"
#include "utils.h"
#include "vec.h"

Parser *parser_create(Lexer *l) {
  Parser *parser = malloc(sizeof(Parser));

  if (!parser)
    error(0, "malloc failed creating parser");

  parser->lexer = l;

  parser->ast = NULL;

  return parser;
}

void parser_free(Parser *p) { free(p); }

void parse(Parser *p, const char *src) {
  lex_init(p->lexer, src);

  Ast *ast =
      parse_block(p); // each block will be considered its own variable
                      // scope, this will allow for shadowing of variable names
  p->ast = ast;
}

Ast *parse_token(Parser *p, Token t) {
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
  case T_VOID:
    lex_next(p->lexer);
    return parse_builtin_type(p, t);
  case T_STRUCT:
  case T_CLASS:
    // TODO: implement class and struct declaration
    return NULL;
  case KW_IF:
    lex_next(p->lexer);
    return parse_if(p);
  case KW_WHILE:
    lex_next(p->lexer);
    return parse_while(p);
  case KW_FOR:
    lex_next(p->lexer);
    return parse_for(p);
  case KW_SWITCH:
    lex_next(p->lexer);
    return parse_switch(p, t);
  default: {
    Ast *expr = parse_expr(p);
    lex_expect_next(p->lexer, TOKEN_SEMICOLON);
    return expr;
  }
  }
}

Ast *parse_i64(Token t) {
  // null terminate the string since its just a const char *
  char buf[64];
  size_t n = t.length < (sizeof(buf) - 1) ? t.length : (sizeof(buf) - 1);
  memcpy(buf, t.start, n);
  buf[n] = 0;
  return ast_int64((i64)strtoll(buf, NULL, 10));
}

Ast *parse_f64(Token t) {
  // null terminate the string since its just a const char *
  char buf[128];
  size_t n = t.length < (sizeof(buf) - 1) ? t.length : (sizeof(buf) - 1);
  memcpy(buf, t.start, n);
  buf[n] = 0;
  return ast_float64(strtod(buf, NULL));
}

Ast *parse_builtin_type(Parser *p, Token tok) {
  int ptr_depth = 0;
  while (lex_peek(p->lexer).type == TOKEN_ASTERISK) {
    ptr_depth++;
    lex_next(p->lexer);
  }

  ModCType *type = type_to_builtin(tok.type);

  if (ptr_depth > 0) {
    type = make_pointer_type(type, ptr_depth);
  }

  tok = lex_peek(p->lexer);

  /*
   * TODO: i think this should only trigger type cast
   * from within the check for TOKEN_LPAREN
   * to ensure that type)[ident];
   * doesnt get registered as a type cast
   */

  // if (tok.type == TOKEN_RPAREN) {
  // This indicates a type cast?
  // (type) [expr]
  // Ast *expr = parse_expr();
  // Ast *ast = ast_cast(); ??
  // return ast;
  // }

  lex_expect(p->lexer, TOKEN_IDENTIFIER);
  tok = lex_next(p->lexer);

  const char *name = tok.start;
  size_t len = tok.length;

  // This will be a declaration
  tok = lex_peek(p->lexer);

  if (tok.type == TOKEN_LPAREN) {
    // function declaration
    // [type] [ident]([params])

    lex_next(p->lexer);

    int param_count = 0;
    Ast **params = parse_function_params(p, &param_count);
    lex_expect_next(p->lexer, TOKEN_RPAREN);
    tok = lex_peek(p->lexer);

    Ast *body = NULL;

    if (tok.type == TOKEN_LCURLY) {
      lex_next(p->lexer);
      body = parse_block(p);
      lex_expect_next(p->lexer, TOKEN_RCURLY);
    } else {
      lex_expect_next(p->lexer, TOKEN_SEMICOLON);
    }

    Ast *ast = ast_funcdecl(name, len, type, params, param_count, body);

    return ast;
  } else {
    // variable declaration
    // [type] [ident]

    Ast *init = NULL;

    if (tok.type == TOKEN_ASSIGN) {
      lex_next(p->lexer);
      init = parse_expr(p);
    }

    Ast *ast = ast_vardecl(name, len, type, init);

    lex_expect_next(p->lexer, TOKEN_SEMICOLON);

    return ast;
  }
}

Ast *parse_ident(Parser *p, Token tok) {
  (void)p;
  Ast *ident = ast_ident(tok.start, tok.length);

  return ident;
}

Ast *parse_if(Parser *p) {
  /*
   * if (cond) {
   *  body
   * } [else] {
   *  else_body
   * }
   */

  lex_expect_next(p->lexer, TOKEN_LPAREN);

  Ast *cond = parse_expr(p);

  lex_expect_next(p->lexer, TOKEN_RPAREN);
  lex_expect_next(p->lexer, TOKEN_LCURLY);

  Ast *body = parse_block(p);

  lex_expect_next(p->lexer, TOKEN_RCURLY);
  Token next_tok = lex_peek(p->lexer);

  Ast *els = NULL;

  if (next_tok.type == KW_ELSE) {
    lex_next(p->lexer);

    lex_expect_next(p->lexer, TOKEN_LCURLY);

    els = parse_block(p);

    lex_expect_next(p->lexer, TOKEN_RCURLY);
  } else {
    lex_expect_next(p->lexer, TOKEN_SEMICOLON);
  }

  Ast *ast = ast_if(cond, body, els);
  return ast;
}

Ast *parse_while(Parser *p) {
  /*
   * while (cond) {
   *  body
   * }
   */

  lex_expect_next(p->lexer, TOKEN_LPAREN);

  Ast *cond = parse_expr(p);

  lex_expect_next(p->lexer, TOKEN_RPAREN);
  lex_expect_next(p->lexer, TOKEN_LCURLY);

  Ast *body = parse_block(p);

  lex_expect_next(p->lexer, TOKEN_RCURLY);
  lex_expect_next(p->lexer, TOKEN_SEMICOLON);

  Ast *ast = ast_while(cond, body);
  return ast;
}

Ast *parse_for(Parser *p) {
  /*
   * for (init; cond; step) {
   *  body
   * }
   */

  lex_expect_next(p->lexer, TOKEN_LPAREN);

  Ast *init = parse_expr(p);

  lex_expect_next(p->lexer, TOKEN_SEMICOLON);

  Ast *cond = parse_expr(p);

  lex_expect_next(p->lexer, TOKEN_SEMICOLON);

  Ast *step = parse_expr(p);

  lex_expect_next(p->lexer, TOKEN_RPAREN);
  lex_expect_next(p->lexer, TOKEN_LCURLY);

  Ast *body = parse_block(p);

  lex_expect_next(p->lexer, TOKEN_RCURLY);
  lex_expect_next(p->lexer, TOKEN_SEMICOLON);

  Ast *ast = ast_for(init, cond, step, body);
  return ast;
}

Ast *parse_switch(Parser *p, Token tok) {
  lex_expect_next(p->lexer, TOKEN_LPAREN);
  (void)tok;
  return NULL;
}

Ast **parse_function_params(Parser *p, int *param_count) {
  /*
   * e.g.
   * type func(type param1; type param2;) { ...; } (with body)
   * type func(type param1;); (without body)
   * type func(type param1 = 100;) { ...; } (declaring default values)
   */

  Vec *params = vec_new(&vec_ast_ptr_type);

  Token next = lex_peek(p->lexer);

  while (next.type != TOKEN_RPAREN) {
    lex_expect_range(p->lexer, T_CHAR8, T_VOID); // expect it to be a type
    lex_next(p->lexer);
    vec_push(params, (void *)parse_builtin_type(p, next));
    next = lex_peek(p->lexer);
  }

  *param_count = params->size;

  Ast **ast_statements = ast_create_array(params->size);
  memmove(ast_statements, params->items, params->size * sizeof(Ast *));

  vec_release(params);

  return ast_statements;
}

Ast *parse_block(Parser *p) {
  /*
   * {
   *  ...;
   *  ...;
   * }
   */

  Vec *statements = vec_new(&vec_ast_ptr_type);

  Token next = lex_peek(p->lexer);

  while (next.type != TOKEN_EOF && next.type != TOKEN_RCURLY) {
    vec_push(statements, (void *)parse_token(p, next));
    next = lex_peek(p->lexer);
  }

  Ast **ast_statements = ast_create_array(statements->size);
  memmove(ast_statements, statements->items, statements->size * sizeof(Ast *));

  Ast *ast = ast_block(ast_statements, statements->size);

  vec_release(statements);

  return ast;
}

Ast *parse_primary(Parser *p) {
  Token t = lex_next(p->lexer);

  switch (t.type) {
  case TOKEN_NUMBER:
    return parse_i64(t);
  case TOKEN_FLOAT:
    return parse_f64(t);
  case TOKEN_STRING:
    return ast_string(t.start, t.length);
  case KW_TRUE:
    return ast_bool(true);
  case KW_FALSE:
    return ast_bool(false);
  case TOKEN_IDENTIFIER:
    return parse_ident(p, t);
  case TOKEN_LPAREN: {
    Ast *expr = parse_expr(p);
    lex_expect_next(p->lexer, TOKEN_RPAREN);
    return expr;
  }
  default:
    error(t.line, "Unexpected token in expression: %s",
          token_type_to_string(t.type));
    return NULL;
  }
}

Ast *parse_prefix(Parser *p) {
  Token t = lex_peek(p->lexer);

  if (t.type == TOKEN_PLUS_PLUS || t.type == TOKEN_MINUS_MINUS ||
      t.type == TOKEN_PLUS || t.type == TOKEN_MINUS || t.type == TOKEN_NOT ||
      t.type == TOKEN_TILDE || t.type == TOKEN_AND ||
      t.type == TOKEN_ASTERISK) {
    lex_next(p->lexer);
    AstUnOp op = ast_toktype_to_unop(t.type);
    Ast *rhs = parse_prefix(p);
    return ast_unop(op, rhs);
  }

  return parse_primary(p);
}

Ast *parse_postfix(Parser *p, Ast *left) {
  for (;;) {
    Token t = lex_peek(p->lexer);

    if (t.type == TOKEN_LPAREN) {
      // function call
      // int arg_count = 0;
      // Ast **args = parse_function_args(p, &arg_count);
      // left = ast_funccall(ident, args, arg_count);
      break;
    }

    if (t.type == TOKEN_PLUS_PLUS || t.type == TOKEN_MINUS_MINUS) {
      lex_next(p->lexer);
      AstUnOp op =
          (t.type == TOKEN_PLUS_PLUS) ? AST_UNOP_POST_INC : AST_UNOP_POST_DEC;
      left = ast_unop(op, left);
      continue;
    }

    break;
  }

  return left;
}

/*
 * Precendence list
 * unary (prefix)      handled in parse_prefix()
 * multiplicative      * / %
 * additive            + -
 * bitwise AND         &
 * bitwise XOR         ^
 * bitwise OR          |
 * shift               << >>
 * relational          < <= > >=
 * equality            == !=
 * logical AND         &&
 * logical OR          ||
 * assignment          = += -= ... (lowest, right-assoc)
 */

static b8 infix_binding_power(TokenType t, int *lbp, int *rbp) {
  switch (t) {
  case TOKEN_ASSIGN:
  case TOKEN_PLUS_ASSIGN:
  case TOKEN_MINUS_ASSIGN:
  case TOKEN_ASTERISK_ASSIGN:
  case TOKEN_SLASH_ASSIGN:
  case TOKEN_MOD_ASSIGN:
  case TOKEN_LESS_LESS_ASSIGN:
  case TOKEN_GREATER_GREATER_ASSIGN:
  case TOKEN_AND_ASSIGN:
  case TOKEN_XOR_ASSIGN:
  case TOKEN_OR_ASSIGN:
    *lbp = 10;
    *rbp = 10;
    return 1;
  case TOKEN_OR_OR:
    *lbp = 20;
    *rbp = 21;
    return 1;
  case TOKEN_AND_AND:
    *lbp = 30;
    *rbp = 31;
    return 1;
  case TOKEN_EQ:
  case TOKEN_NOT_EQUALS:
    *lbp = 40;
    *rbp = 41;
    return 1;
  case TOKEN_LESS:
  case TOKEN_LESS_ASSIGN:
  case TOKEN_GREATER:
  case TOKEN_GREATER_ASSIGN:
    *lbp = 50;
    *rbp = 51;
    return 1;
  case TOKEN_OR:
    *lbp = 60;
    *rbp = 61;
    return 1;
  case TOKEN_XOR:
    *lbp = 70;
    *rbp = 71;
    return 1;
  case TOKEN_AND:
    *lbp = 80;
    *rbp = 81;
    return 1;
  case TOKEN_PLUS:
  case TOKEN_MINUS:
    *lbp = 90;
    *rbp = 91;
    return 1;
  case TOKEN_LESS_LESS:
  case TOKEN_GREATER_GREATER:
    *lbp = 85;
    *rbp = 86;
    return 1;
  case TOKEN_ASTERISK:
  case TOKEN_SLASH:
  case TOKEN_MOD:
    *lbp = 100;
    *rbp = 101;
    return 1;
  default:
    break;
  }

  return 0;
}

Ast *parse_expr_bp(Parser *p, int min_bp) {
  Ast *left = parse_prefix(p);
  left = parse_postfix(p, left);

  for (;;) {
    Token t = lex_peek(p->lexer);

    int lbp, rbp;
    AstBinOp op = ast_toktype_to_binop(t.type);

    if (!infix_binding_power(t.type, &lbp, &rbp))
      break;
    if (lbp < min_bp)
      break;

    lex_next(p->lexer);
    Ast *right = parse_expr_bp(p, rbp);

    int is_err = 0;
    left = ast_binop(op, left, right, &is_err);
    if (is_err) {
      error(0, "Invalid binary operation in expression");
      return NULL;
    }
  }

  return left;
}

Ast *parse_expr(Parser *p) { return parse_expr_bp(p, 0); }
