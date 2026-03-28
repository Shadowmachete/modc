#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

#include "types.h"

typedef enum {
  // builtin types
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
  TOKEN_EQ,     // ==
  TOKEN_ASSIGN, // =

  TOKEN_PLUS,        // +
  TOKEN_PLUS_PLUS,   // ++
  TOKEN_PLUS_ASSIGN, // +=

  TOKEN_MINUS,        // -
  TOKEN_MINUS_MINUS,  // --
  TOKEN_MINUS_ASSIGN, // -=

  TOKEN_ASTERISK,        // *
  TOKEN_ASTERISK_ASSIGN, // *=

  TOKEN_SLASH,        // /
  TOKEN_SLASH_ASSIGN, // /=

  TOKEN_MOD,        // %
  TOKEN_MOD_ASSIGN, // %=

  TOKEN_XOR,        // ^
  TOKEN_XOR_ASSIGN, // ^=

  TOKEN_COLON,     // :
  TOKEN_SEMICOLON, // ;
  TOKEN_COMMA,     // ,
  TOKEN_LPAREN,    // (
  TOKEN_RPAREN,    // )
  TOKEN_LCURLY,    // {
  TOKEN_RCURLY,    // }
  TOKEN_LSQUARE,   // [
  TOKEN_RSQUARE,   // ]

  TOKEN_LESS,             // <
  TOKEN_LESS_LESS,        // <<
  TOKEN_LESS_ASSIGN,      // <=
  TOKEN_LESS_LESS_ASSIGN, // <<=

  TOKEN_GREATER,                // >
  TOKEN_GREATER_GREATER,        // >>
  TOKEN_GREATER_ASSIGN,         // >=
  TOKEN_GREATER_GREATER_ASSIGN, // >>=

  TOKEN_AND,        // &
  TOKEN_AND_AND,    // &&
  TOKEN_AND_ASSIGN, // &=

  TOKEN_OR,        // |
  TOKEN_OR_OR,     // ||
  TOKEN_OR_ASSIGN, // |=

  TOKEN_NOT,        // !
  TOKEN_TILDE,      // ~
  TOKEN_NOT_EQUALS, // !=

  // TODO: add more

  // control flow
  KW_IF,
  KW_ELSE,
  KW_WHILE,
  KW_FOR,
  KW_SWITCH,
  KW_CASE,
  KW_GOTO,
  KW_RETURN,

  // variables and values
  TOKEN_NUMBER,
  TOKEN_FLOAT,
  TOKEN_STRING,
  TOKEN_IDENTIFIER,

  TOKEN_EOF,
  TOKEN_UNKNOWN,
} TokenType;

const char *token_type_to_string(TokenType t);

typedef struct {
  const char *start;
  size_t length;
  size_t line;
  TokenType type;
} Token;

void token_print(Token t);

typedef struct {
  const char *src;
  size_t pos;
  size_t line;

  Token peeked;
  b8 has_peeked;
} Lexer;

void lex_init(Lexer *l, const char *src);
Token lex_next(Lexer *l);
Token lex_peek(Lexer *l);
void lex_expect(Lexer *l, TokenType tok_type);
void lex_expect_range(Lexer *l, TokenType start, TokenType end);
void lex_expect_next(Lexer *l, TokenType tok_type);
Token lex_number(Lexer *l, char *c, const char *start);
void lex_comment(Lexer *l, char *c);
Token lex_ident(Lexer *l, char *c, const char *start);

#endif // !LEXER_H
