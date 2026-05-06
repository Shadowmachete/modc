#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

#include "ast.h"
#include "lexer.h"

typedef struct Parser Parser;

struct Parser {
  Ast *ast;
  Lexer *lexer;
};

Parser *parser_create(Lexer *l);
void parser_free(Parser *p);

void parse(Parser *p, const char *src);
Ast *parse_token(Parser *p, Token t);

Ast *parse_i64(Token t);
Ast *parse_f64(Token t);

Ast *parse_builtin_type(Parser *p, Token tok);
Ast *parse_ident(Parser *p, Token tok);
Ast *parse_if(Parser *p, Token tok);
Ast *parse_while(Parser *p, Token tok);
Ast *parse_for(Parser *p, Token tok);
Ast *parse_switch(Parser *p, Token tok);
Ast *parse_return(Parser *p, Token tok);

Ast **parse_function_params(Parser *p, int *param_count);
Ast *parse_param(Parser *p, Token tok);
Ast **parse_function_args(Parser *p, int *arg_count);
Ast *parse_arg(Parser *p, Token tok);

Ast *parse_block(Parser *p);
Ast *parse_expr(Parser *p);

#endif // !PARSER_H
