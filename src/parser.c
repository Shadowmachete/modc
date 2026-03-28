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
    return parse_builtin_type(p, t);
  case T_STRUCT:
  case T_CLASS:
    // TODO: implement class and struct declaration
    return NULL;
  case TOKEN_IDENTIFIER:
    return parse_ident(p, t);
  case KW_IF:
    return parse_if(p);
  case KW_WHILE:
    return parse_while(p);
  case KW_FOR:
    return parse_for(p);
  case KW_SWITCH:
    return parse_switch(p, t);
  default:
    return NULL;
  }

  // TODO: handle literals: string, float, int, bool
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

  if (tok.type == TOKEN_RPAREN) {
    // This indicates a type cast?
    // (type) [expr]
    // Ast *expr = parse_expr();
    // Ast *ast = ast_cast(); ??
    // return ast;
  }

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
      /* init = parse_expr(p); */
    }

    Ast *ast = ast_vardecl(name, len, type, init);

    lex_expect_next(p->lexer, TOKEN_SEMICOLON);

    return ast;
  }
}

Ast *parse_ident(Parser *p, Token tok) {
  Token next_tok = lex_peek(p->lexer);

  Ast *ident = ast_ident(tok.start, tok.length);

  if (next_tok.type == TOKEN_RPAREN) {
    // function call
    // int arg_count = 0;
    // Ast **args = parse_function_args(p, &arg_count);
    // Ast *ast = ast_funccall(ident, args, arg_count);
    // return ast;
  }

  // cases where an identifier is just used
  // - operations, ident + 5
  // - assignment, ident = 1 + 5
  // TODO: handle binary and unary operations
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

  /* Ast *cond = parse_expr(p); */

  lex_expect_next(p->lexer, TOKEN_RPAREN);
  lex_expect_next(p->lexer, TOKEN_LCURLY);

  /* Ast *body = parse_block(p); */

  lex_expect_next(p->lexer, TOKEN_RCURLY);
  Token next_tok = lex_peek(p->lexer);

  /* Ast *els = NULL; */

  if (next_tok.type == KW_ELSE) {
    lex_next(p->lexer);

    lex_expect_next(p->lexer, TOKEN_LCURLY);

    /* els = parse_block(p); */

    lex_expect_next(p->lexer, TOKEN_RCURLY);
  } else {
    lex_expect_next(p->lexer, TOKEN_SEMICOLON);
  }

  /* Ast *ast = ast_if(cond, body, els); */
  return NULL;
}

Ast *parse_while(Parser *p) {
  /*
   * while (cond) {
   *  body
   * }
   */

  lex_expect_next(p->lexer, TOKEN_LPAREN);

  /* Ast *cond = parse_expr(p); */

  lex_expect_next(p->lexer, TOKEN_RPAREN);
  lex_expect_next(p->lexer, TOKEN_LCURLY);

  /* Ast *body = parse_block(p); */

  lex_expect_next(p->lexer, TOKEN_RCURLY);
  lex_expect_next(p->lexer, TOKEN_SEMICOLON);

  /* Ast *ast = ast_while(cond, body); */
  return NULL;
}

Ast *parse_for(Parser *p) {
  /*
   * for (init; cond; step) {
   *  body
   * }
   */
  lex_expect_next(p->lexer, TOKEN_LPAREN);

  /* Ast *init = parse_expr(p); */

  lex_expect_next(p->lexer, TOKEN_SEMICOLON);

  /* Ast *cond = parse_expr(p); */

  lex_expect_next(p->lexer, TOKEN_SEMICOLON);

  /* Ast *step = parse_expr(p); */

  lex_expect_next(p->lexer, TOKEN_RPAREN);
  lex_expect_next(p->lexer, TOKEN_LCURLY);

  /* Ast *body = parse_block(p) */

  lex_expect_next(p->lexer, TOKEN_RCURLY);
  lex_expect_next(p->lexer, TOKEN_SEMICOLON);

  /* Ast *ast = ast_for(init, cond, step, body); */
  return NULL;
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
    lex_next(p->lexer);
    vec_push(statements, (void *)parse_token(p, next));
    next = lex_peek(p->lexer);
  }

  Ast **ast_statements = ast_create_array(statements->size);
  memmove(ast_statements, statements->items, statements->size * sizeof(Ast *));

  Ast *ast = ast_block(ast_statements, statements->size);

  vec_release(statements);

  return ast;
}

Ast *parse_expr(Parser *p) {
  /*
    TODO: parse different types of expressions
    e.g.
    x = [expr];
    if (expr)
    for (expr; expr; expr)
    return [expr];
   */
  (void)p;
  return NULL;
}
