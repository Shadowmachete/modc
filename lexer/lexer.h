#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  // types
  T_CHAR8,
  T_CHAR16,
  T_CHAR32,

  T_UINT8,
  T_UINT16,
  T_UINT32,
  T_UINT64,

  T_INT8,
  T_INT16,
  T_INT32,
  T_INT64,

  T_FLOAT16,
  T_FLOAT32,
  T_FLOAT64,

  T_BOOL,

  T_STRUCT,
  T_CLASS,

  T_VOID,

  // symbols
  TOKEN_EQUALS,
  TOKEN_ASSIGN,

  TOKEN_PLUS,
  TOKEN_PLUS_EQUALS,
  TOKEN_PLUS_PLUS,

  TOKEN_MINUS,
  TOKEN_MINUS_EQUALS,
  TOKEN_MINUS_MINUS,

  TOKEN_ASTERIX,
  TOKEN_TIMES_EQUALS,

  TOKEN_SLASH,
  TOKEN_DIVIDE_EQUALS,

  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,

  TOKEN_LESS,
  TOKEN_SHL,
  TOKEN_LESS_EQUALS,

  TOKEN_GREATER,
  TOKEN_SHR,
  TOKEN_GREATER_EQUALS,

  TOKEN_BITWISE_AND,
  TOKEN_LOGICAL_AND,
  TOKEN_BITWISE_AND_EQUALS,
  TOKEN_LOGICAL_AND_EQUALS,

  TOKEN_BITWISE_OR,
  TOKEN_LOGICAL_OR,
  TOKEN_BITWISE_OR_EQUALS,
  TOKEN_LOGICAL_OR_EQUALS,

  TOKEN_NOT,
  TOKEN_NOT_EQUALS,

  // control flow
  KW_IF,
  KW_ELSE,
  KW_WHILE,
  KW_FOR,
  KW_SWITCH,
  KW_CASE,
  KW_GOTO,
  KW_RETURN,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LCURLY,
  TOKEN_RCURLY,

  // variables and values
  TOKEN_IDENTIFIER,
  TOKEN_NUMBER,
  TOKEN_FLOAT,
  TOKEN_STRING,

  TOKEN_EOF,
  TOKEN_UNKNOWN,
} TokenType;

typedef struct {
  const char *start;
  size_t length;
  size_t line;
  TokenType type;
} Token;

typedef struct {
  const char *src;
  size_t pos;
  size_t line;

  Token peeked;
  bool has_peeked;
} Lexer;

void lex_init(Lexer *l, const char *src);
Token lex_next(Lexer *l);
Token lex_peek(Lexer *l);
Token lex_number(Lexer *l, char *c, const char *start);
void lex_comment(Lexer *l, char *c);
Token lex_ident(Lexer *l, char *c, const char *start);

const char *token_type_name(TokenType t);
void token_print(Token t);

#endif
