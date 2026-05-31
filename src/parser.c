#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "modc_types.h"
#include "parser.h"
#include "utils.h"
#include "vec.h"

#define expr_expect                                                            \
  if (!lex_expect_(p->lexer, TOKEN_SEMICOLON, &tok_actual)) {                  \
    error(p->lexer->f, tok_actual.line_info,                                   \
          "expected ';' after expression, got '%s'",                           \
          token_type_to_cstr(tok_actual.type));                                \
  } else {                                                                     \
    lex_next(p->lexer);                                                        \
  }

Token tok_actual;

Parser *parser_create(Lexer *l) {
  Parser *parser = malloc(sizeof(Parser));

  if (!parser)
    error(NULL, (LineInfo){0}, "malloc failed creating parser");

  parser->lexer = l;

  parser->ast = NULL;

  return parser;
}

void parser_free(Parser *p) { free(p); }

void parse(Parser *p, File *f) {
  lex_init(p->lexer, f);

  Ast *ast = parse_block(p);
  p->ast = ast;
}

Ast *parse_token(Parser *p, Token t) {
  // TODO: implement more keywords for parsing into ast

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
  case T_VOID: {
    lex_next(p->lexer);
    return parse_builtin_type(p, t);
  }
  case T_STRUCT:
    return NULL;
  case KW_IF: {
    lex_next(p->lexer);
    return parse_if(p, t);
  }
  case KW_ELSE: {
    lex_next(p->lexer);
    error(p->lexer->f, t.line_info,
          "encountered token 'ELSE' outside of if-else group");

    return NULL;
  }
  case KW_WHILE: {
    lex_next(p->lexer);
    return parse_while(p, t);
  }
  case KW_FOR: {
    lex_next(p->lexer);
    return parse_for(p, t);
  }
  case KW_SWITCH: {
    lex_next(p->lexer);
    return parse_switch(p, t);
  }
  case KW_RETURN: {
    lex_next(p->lexer);
    return parse_return(p, t);
  }
  case TOKEN_LCURLY: {
    lex_next(p->lexer);
    return parse_block(p);
  }
  case TOKEN_NEWLINE: {
    lex_next(p->lexer);
    return parse_token(p, lex_peek(p->lexer));
  }
  default: {
    Ast *expr = parse_expr(p);
    expr_expect;
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
  Ast *ast = ast_int64((i64)strtoll(buf, NULL, 10));
  ast->line_info = t.line_info;
  return ast;
}

Ast *parse_f64(Token t) {
  // null terminate the string since its just a const char *
  char buf[128];
  size_t n = t.length < (sizeof(buf) - 1) ? t.length : (sizeof(buf) - 1);
  memcpy(buf, t.start, n);
  buf[n] = 0;
  Ast *ast = ast_float64(strtod(buf, NULL));
  ast->line_info = t.line_info;
  return ast;
}

Ast *parse_builtin_type(Parser *p, Token tok) {
  int ptr_depth = 0;
  while (lex_peek(p->lexer).type == TOKEN_ASTERISK) {
    ptr_depth++;
    lex_next(p->lexer);
  }

  ModCType *type = tok_type_to_modctype(tok.type);

  if (ptr_depth > 0) {
    type = make_pointer_type(type, ptr_depth);
  }

  const char *name;
  size_t len;

  if (!lex_expect_(p->lexer, TOKEN_IDENTIFIER, &tok_actual)) {
    error(p->lexer->f, tok_actual.line_info,
          "expected identifier after type, got '%s'",
          token_type_to_cstr(tok_actual.type));
  } else {
    tok = lex_next(p->lexer);
    name = tok.start;
    len = tok.length;
  }

  // This will be a declaration
  tok = lex_peek(p->lexer);
  LineInfo decl_line_info = tok.line_info;

  if (tok.type == TOKEN_LPAREN) {
    // function declaration
    // [type] [ident]([params]) { body }

    lex_next(p->lexer);

    int param_count = 0;
    Ast **params = parse_function_params(p, &param_count);
    if (lex_peek(p->lexer).type == TOKEN_EOF) {
      error(p->lexer->f, tok.line_info, "function param never closes");

      return NULL;
    }

    if (!lex_expect_(p->lexer, TOKEN_RPAREN, &tok_actual)) {
      error(p->lexer->f, tok_actual.line_info, "expected ')' before '%s'",
            token_type_to_cstr(tok_actual.type));
    } else {
      lex_next(p->lexer);
    }

    tok = lex_peek(p->lexer);

    // NOTE: the type of the first parameter defined will be given a "class"
    // method so that you can either call this function as func(a, b) or
    // a.func(b)
    // I will not be supporting operator overloading (a + b will have to be
    // add(a, b) or a.add(b))
    //
    // TODO: mangle function names with their type so that func(int, ...) and
    // func(float, ...) can be defined for each of those types
    // this can make it easier to look up the presence of a function in the
    // symbol table too

    if (!lex_expect_(p->lexer, TOKEN_LCURLY, &tok_actual)) {
      error(p->lexer->f, tok_actual.line_info,
            "expected function body after function declarator");
    } else {
      lex_next(p->lexer);
    }

    Ast *body = parse_block(p);

    if (lex_peek(p->lexer).type == TOKEN_EOF) {
      error(p->lexer->f, tok.line_info, "function block never closes");
      return NULL;
    }

    if (!lex_expect_(p->lexer, TOKEN_RCURLY, &tok_actual)) {
      error(p->lexer->f, tok_actual.line_info,
            "expected end of function body, got '%s'",
            token_type_to_cstr(tok_actual.type));
    } else {
      lex_next(p->lexer);
    }

    Ast *ast = ast_funcdecl(name, len, type, params, param_count, body);

    ast->line_info = decl_line_info;

    return ast;
  } else {
    // variable declaration
    // [type] [ident] [= init], ...;

    Ast *init = NULL;

    if (tok.type == TOKEN_ASSIGN) {
      lex_next(p->lexer);
      init = parse_expr(p);
    }
    // else {
    //   init = make_default_init(type);
    // }

    // TODO: change ast to contain a array of var decls to store multiple decls

    Ast *ast = ast_vardecl(name, len, type, init);
    ast->line_info = decl_line_info;

    if (!lex_expect_(p->lexer, TOKEN_SEMICOLON, &tok_actual)) {
      error(p->lexer->f, tok_actual.line_info,
            "expected ';' after variable declaration");
    } else {
      lex_next(p->lexer);
    }

    return ast;
  }
}

Ast *parse_ident(Parser *p, Token tok) {
  (void)p;
  Ast *ident = ast_ident(tok.start, tok.length);
  ident->line_info = tok.line_info;

  return ident;
}

Ast *parse_if(Parser *p, Token tok) {
  /*
   * if (cond) {
   *  body
   * } [else] [[if] (cond)] {
   *  else_body
   * }
   */

  if (!lex_expect_(p->lexer, TOKEN_LPAREN, &tok_actual)) {
    error(p->lexer->f, tok_actual.line_info, "expected '(' after if");
  } else {
    lex_next(p->lexer);
  }

  Ast *cond = parse_expr(p);

  if (!lex_expect_(p->lexer, TOKEN_RPAREN, &tok_actual)) {
    error(p->lexer->f, tok_actual.line_info, "expected ')'");
  } else {
    lex_next(p->lexer);
  }

  Ast *then;

  if (lex_peek(p->lexer).type == TOKEN_LCURLY) {
    lex_next(p->lexer);
    then = parse_block(p);

    if (!lex_expect_(p->lexer, TOKEN_RCURLY, &tok_actual)) {
      error(p->lexer->f, tok.line_info, "if block never closes");
    } else {
      lex_next(p->lexer);
    }
  } else {
    if (lex_peek(p->lexer).type == TOKEN_NEWLINE) {
      lex_next(p->lexer);
    }
    then = parse_expr(p);

    expr_expect;
  }

  if (lex_peek(p->lexer).type == TOKEN_NEWLINE) {
    lex_next(p->lexer);
  }

  Token next_tok = lex_peek(p->lexer);

  Ast *els = NULL;

  if (next_tok.type == KW_ELSE) {
    lex_next(p->lexer);

    if (lex_peek(p->lexer).type == KW_IF) {
      els = parse_if(p, lex_next(p->lexer));
    } else {
      if (lex_peek(p->lexer).type == TOKEN_LCURLY) {
        lex_next(p->lexer);
        els = parse_block(p);
        if (!lex_expect_(p->lexer, TOKEN_RCURLY, &tok_actual)) {
          error(p->lexer->f, tok.line_info, "else block never closes");
        } else {
          lex_next(p->lexer);
        }
      } else {
        if (lex_peek(p->lexer).type == TOKEN_NEWLINE) {
          lex_next(p->lexer);
        }
        els = parse_expr(p);

        expr_expect;
      }
    }
  }

  Ast *ast = ast_if(cond, then, els);
  ast->line_info = tok.line_info;

  return ast;
}

Ast *parse_while(Parser *p, Token tok) {
  /*
   * while (cond) {
   *  body
   * }
   */

  if (!lex_expect_(p->lexer, TOKEN_LPAREN, &tok_actual)) {
    error(p->lexer->f, tok_actual.line_info, "expected '(' after while");
  } else {
    lex_next(p->lexer);
  }

  Ast *cond = parse_expr(p);

  if (!lex_expect_(p->lexer, TOKEN_RPAREN, &tok_actual)) {
    error(p->lexer->f, tok_actual.line_info, "expected ')'");
  } else {
    lex_next(p->lexer);
  }

  Ast *body;

  if (lex_peek(p->lexer).type == TOKEN_LCURLY) {
    lex_next(p->lexer);
    body = parse_block(p);

    if (!lex_expect_(p->lexer, TOKEN_RCURLY, &tok_actual)) {
      error(p->lexer->f, tok.line_info, "while block never closes");
    } else {
      lex_next(p->lexer);
    }
  } else {
    if (lex_peek(p->lexer).type == TOKEN_NEWLINE) {
      lex_next(p->lexer);
    }
    body = parse_expr(p);

    expr_expect;
  }

  Ast *ast = ast_while(cond, body);
  ast->line_info = tok.line_info;

  return ast;
}

Ast *parse_for(Parser *p, Token tok) {
  /*
   * for (init; cond; step) {
   *  body
   * }
   */

  LineInfo line_info = tok.line_info;

  // TODO: change to a conditional when adding support for different forms of
  // for loop (i.e. iterator based)
  if (!lex_expect_(p->lexer, TOKEN_LPAREN, &tok_actual)) {
    error(p->lexer->f, tok_actual.line_info, "expected '(' after for");
  } else {
    lex_next(p->lexer);
  }

  Ast *init = NULL, *cond = NULL, *step = NULL;

  if (!lex_expect_range_(p->lexer, T_CHAR8, T_VOID, &tok_actual) &&
      tok_actual.type != TOKEN_SEMICOLON) {
    error(p->lexer->f, tok_actual.line_info,
          "expected type declaration, got '%s'",
          token_type_to_cstr(tok_actual.type));
  } else if (lex_peek(p->lexer).type != TOKEN_SEMICOLON) {
    tok = lex_next(p->lexer);
    init = parse_builtin_type(p, tok);
  }

  if (lex_peek(p->lexer).type != TOKEN_SEMICOLON) {
    cond = parse_expr(p);
    expr_expect;
  } else {
    lex_next(p->lexer);
  }

  if (lex_peek(p->lexer).type != TOKEN_RPAREN) {
    step = parse_expr(p);
  }

  if (!lex_expect_(p->lexer, TOKEN_RPAREN, &tok_actual)) {
    error(p->lexer->f, tok_actual.line_info, "expected ')'");
  } else {
    lex_next(p->lexer);
  }

  Ast *body;

  if (lex_peek(p->lexer).type == TOKEN_LCURLY) {
    lex_next(p->lexer);
    body = parse_block(p);

    if (!lex_expect_(p->lexer, TOKEN_RCURLY, &tok_actual)) {
      error(p->lexer->f, line_info, "for block never closes");
    } else {
      lex_next(p->lexer);
    }
  } else {
    if (lex_peek(p->lexer).type == TOKEN_NEWLINE) {
      lex_next(p->lexer);
    }
    body = parse_expr(p);

    expr_expect;
  }

  Ast *ast = ast_for(init, cond, step, body);
  ast->line_info = line_info;

  return ast;
}

Ast *parse_switch(Parser *p, Token tok) {
  if (!lex_expect_(p->lexer, TOKEN_LPAREN, &tok_actual)) {
    error(p->lexer->f, tok_actual.line_info, "expected '(' after switch");
  } else {
    lex_next(p->lexer);
  }
  (void)tok;
  return NULL;
}

Ast *parse_return(Parser *p, Token tok) {
  Ast *expr = NULL;

  if (lex_peek(p->lexer).type != TOKEN_SEMICOLON) {
    expr = parse_expr(p);

    expr_expect;
  }

  Ast *return_stmt = ast_return(expr);
  return_stmt->line_info = tok.line_info;

  return return_stmt;
}

Ast **parse_function_params(Parser *p, int *param_count) {
  /*
   * e.g.
   * type func(type param1; type param2;) { ...; } (with body)
   * type func(type param1 = 100;) { ...; } (declaring default values)
   */

  Vec *params = vec_new(&vec_ast_ptr_type);

  Token next = lex_peek(p->lexer);

  while (next.type != TOKEN_RPAREN) {
    if (!lex_expect_range_(p->lexer, T_CHAR8, T_VOID, &tok_actual) &&
        tok_actual.type != TOKEN_SEMICOLON) {
      error(p->lexer->f, tok_actual.line_info,
            "expected type declaration, got '%s'",
            token_type_to_cstr(tok_actual.type));

      vec_release(params);
      return NULL;
    }
    lex_next(p->lexer);
    vec_push(params, (void *)parse_param(p, next));
    next = lex_peek(p->lexer);

    if (next.type == TOKEN_EOF) {
      break;
    }
  }

  *param_count = params->size;

  Ast **ast_statements = ast_create_array(params->size);
  memmove(ast_statements, params->items, params->size * sizeof(Ast *));

  vec_release(params);

  return ast_statements;
}

Ast *parse_param(Parser *p, Token tok) {
  LineInfo param_line_info = tok.line_info;

  int ptr_depth = 0;
  while (lex_peek(p->lexer).type == TOKEN_ASTERISK) {
    ptr_depth++;
    lex_next(p->lexer);
  }

  ModCType *type = tok_type_to_modctype(tok.type);

  if (ptr_depth > 0) {
    type = make_pointer_type(type, ptr_depth);
  }

  if (!lex_expect_(p->lexer, TOKEN_IDENTIFIER, &tok_actual)) {
    error(p->lexer->f, tok_actual.line_info,
          "expected identifier for param, got '%s'",
          token_type_to_cstr(tok_actual.type));

    return NULL;
  }

  tok = lex_next(p->lexer);

  const char *name = tok.start;
  size_t len = tok.length;

  Ast *init = NULL;

  if (lex_peek(p->lexer).type == TOKEN_ASSIGN) {
    lex_next(p->lexer);
    init = parse_expr(p);
  }

  Ast *ast = ast_vardecl(name, len, type, init);
  ast->line_info = param_line_info;

  expr_expect;

  return ast;
}

Ast **parse_function_args(Parser *p, int *arg_count) {
  Vec *args = vec_new(&vec_ast_ptr_type);

  Token next = lex_peek(p->lexer);

  while (next.type != TOKEN_RPAREN) {
    if (next.type == TOKEN_COMMA) {
      lex_next(p->lexer);
      next = lex_peek(p->lexer);
      continue;
    }
    vec_push(args, (void *)parse_expr(p));
    next = lex_peek(p->lexer);
  }

  *arg_count = args->size;

  Ast **ast_statements = ast_create_array(args->size);
  memmove(ast_statements, args->items, args->size * sizeof(Ast *));

  vec_release(args);

  return ast_statements;
}

Ast *parse_block(Parser *p) {
  /*
   * {
   *  ...;
   *  ...;
   * }
   *
   * TODO: last statement in any block will be assigned as the block's return
   * value if no semicolon is provided, else the return of the block is void
   */

  Vec *statements = vec_new(&vec_ast_ptr_type);

  Token next = lex_peek(p->lexer);

  while (next.type != TOKEN_EOF && next.type != TOKEN_RCURLY) {
    if (next.type == TOKEN_NEWLINE) {
      lex_next(p->lexer);
      next = lex_peek(p->lexer);
      continue;
    }

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
  case TOKEN_STRING: {
    Ast *str = ast_string(t.start, t.length);
    str->line_info = t.line_info;

    return str;
  }
  case KW_TRUE: {
    Ast *tru = ast_bool(true);
    tru->line_info = t.line_info;

    return tru;
  }
  case KW_FALSE: {
    Ast *fals = ast_bool(false);
    fals->line_info = t.line_info;

    return fals;
  }
  case TOKEN_IDENTIFIER:
    return parse_ident(p, t);
  case TOKEN_LPAREN: {
    Ast *expr = parse_expr(p);
    if (!lex_expect_next_(p->lexer, TOKEN_RPAREN, &tok_actual)) {
      error(p->lexer->f, tok_actual.line_info, "expected ')', got '%s'",
            token_type_to_cstr(tok_actual.type));

      return NULL;
    }
    return expr;
  }
  default:
    error(p->lexer->f, t.line_info, "Unexpected token in expression: %s",
          token_type_to_cstr(t.type));
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
    Ast *unop = ast_unop(op, rhs);
    unop->line_info = t.line_info;

    return unop;
  }

  return parse_primary(p);
}

Ast *parse_postfix(Parser *p, Ast *left) {
  for (;;) {
    Token t = lex_peek(p->lexer);

    if (t.type == TOKEN_LPAREN) {
      // function call
      lex_next(p->lexer);
      int arg_count = 0;
      Ast **args = parse_function_args(p, &arg_count);
      if (!lex_expect_next_(p->lexer, TOKEN_RPAREN, &tok_actual)) {
        error(p->lexer->f, tok_actual.line_info,
              "expected ')' at the end of function call, got '%s'",
              token_type_to_cstr(tok_actual.type));

        return NULL;
      }
      left = ast_funccall(left, args, arg_count);
      left->line_info = t.line_info;

      break;
    }

    if (t.type == TOKEN_PLUS_PLUS || t.type == TOKEN_MINUS_MINUS) {
      lex_next(p->lexer);
      AstUnOp op =
          (t.type == TOKEN_PLUS_PLUS) ? AST_UNOP_POST_INC : AST_UNOP_POST_DEC;
      left = ast_unop(op, left);
      left->line_info = t.line_info;

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
 * shift               << >>
 * bitwise AND         &
 * bitwise XOR         ^
 * bitwise OR          |
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
  case TOKEN_LESS_LESS:
  case TOKEN_GREATER_GREATER:
    *lbp = 85;
    *rbp = 86;
    return 1;
  case TOKEN_PLUS:
  case TOKEN_MINUS:
    *lbp = 90;
    *rbp = 91;
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
    left->line_info = t.line_info;

    if (is_err) {
      error(p->lexer->f, t.line_info, "Invalid binary operation in expression");
      return NULL;
    }
  }

  return left;
}

Ast *parse_expr(Parser *p) { return parse_expr_bp(p, 0); }
