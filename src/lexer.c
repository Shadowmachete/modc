#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "types.h"
#include "utils.h"

void lex_init(Lexer *l, File *f) {
  l->pos = 0;
  l->line_offset = f->src;
  l->line = 1;
  l->f = f;
  l->has_peeked = 0;
}

Token lex_next(Lexer *l) {
  if (l->has_peeked) {
    l->has_peeked = false;
    return l->peeked;
  }

  char *c;

again:
  c = (char *)l->f->src + l->pos;
  while (*c == ' ' || *c == '\t') {
    c++;
  }

  const char *start = c;

  if (*c == '\0')
    return (Token){start, 0,
                   (LineInfo){l->line_offset, l->line, c - l->line_offset},
                   TOKEN_EOF};

  if (*c == '\n') {
    Token tok = (Token){start, 1,
                        (LineInfo){l->line_offset, l->line, c - l->line_offset},
                        TOKEN_NEWLINE};
    l->line++;
    l->line_offset = ++c;
    l->pos = c - l->f->src;

    return tok;
  }

  if (isdigit((unsigned char)*c))
    return lex_number(l, c, start);

  if (isalpha((unsigned char)*c) || *c == '_')
    return lex_ident(l, c, start);

  Token tok;

  switch (*c) {
  case '=':
    if (*(c + 1) == '=') {
      c++;
      tok.type = TOKEN_EQ;
    } else {
      tok.type = TOKEN_ASSIGN;
    }
    break;
  case '+':
    if (*(c + 1) == '=') {
      c++;
      tok.type = TOKEN_PLUS_ASSIGN;
    } else if (*(c + 1) == '+') {
      c++;
      tok.type = TOKEN_PLUS_PLUS;
    } else {
      tok.type = TOKEN_PLUS;
    }
    break;
  case '-':
    if (*(c + 1) == '=') {
      c++;
      tok.type = TOKEN_MINUS_ASSIGN;
    } else if (*(c + 1) == '-') {
      c++;
      tok.type = TOKEN_MINUS_MINUS;
    } else {
      tok.type = TOKEN_MINUS;
    }
    break;
  case '*':
    if (*(c + 1) == '=') {
      c++;
      tok.type = TOKEN_ASTERISK_ASSIGN;
    } else {
      tok.type = TOKEN_ASTERISK;
    }
    break;
  case '%':
    if (*(c + 1) == '=') {
      c++;
      tok.type = TOKEN_MOD_ASSIGN;
    } else {
      tok.type = TOKEN_MOD;
    }
    break;
  case '^':
    if (*(c + 1) == '=') {
      c++;
      tok.type = TOKEN_XOR_ASSIGN;
    } else {
      tok.type = TOKEN_XOR;
    }
    break;
  case '/':
    if (*(c + 1) == '/') {
      lex_comment(l, c); // only '//' comments for now
      goto again;
    } else if (*(c + 1) == '=') {
      c++;
      tok.type = TOKEN_SLASH_ASSIGN;
    } else {
      tok.type = TOKEN_SLASH;
    }
    break;
  case ':':
    tok.type = TOKEN_COLON;
    break;
  case ';':
    tok.type = TOKEN_SEMICOLON;
    break;
  case ',':
    tok.type = TOKEN_COMMA;
    break;
  case '(':
    tok.type = TOKEN_LPAREN;
    break;
  case ')':
    tok.type = TOKEN_RPAREN;
    break;
  case '{':
    tok.type = TOKEN_LCURLY;
    break;
  case '}':
    tok.type = TOKEN_RCURLY;
    break;
  case '[':
    tok.type = TOKEN_LSQUARE;
    break;
  case ']':
    tok.type = TOKEN_RSQUARE;
    break;
  case '~':
    tok.type = TOKEN_TILDE;
    break;
  case '<':
    if (*(c + 1) == '<') {
      c++;
      if (*(c + 1) == '=') {
        c++;
        tok.type = TOKEN_LESS_LESS_ASSIGN;
      } else {
        tok.type = TOKEN_LESS_LESS;
      }
    } else {
      if (*(c + 1) == '=') {
        c++;
        tok.type = TOKEN_LESS_ASSIGN;
      } else {
        tok.type = TOKEN_LESS;
      }
    }
    break;
  case '>':
    if (*(c + 1) == '>') {
      c++;
      if (*(c + 1) == '=') {
        c++;
        tok.type = TOKEN_GREATER_GREATER_ASSIGN;
      } else {
        tok.type = TOKEN_GREATER_GREATER;
      }
    } else {
      if (*(c + 1) == '=') {
        c++;
        tok.type = TOKEN_GREATER_ASSIGN;
      } else {
        tok.type = TOKEN_GREATER;
      }
    }
    break;
  case '&':
    if (*(c + 1) == '&') {
      c++;
      tok.type = TOKEN_AND_AND;
    } else {
      if (*(c + 1) == '=') {
        c++;
        tok.type = TOKEN_AND_ASSIGN;
      } else {
        tok.type = TOKEN_AND;
      }
    }
    break;
  case '|':
    if (*(c + 1) == '|') {
      c++;
      tok.type = TOKEN_OR_OR;
    } else {
      if (*(c + 1) == '=') {
        c++;
        tok.type = TOKEN_OR_ASSIGN;
      } else {
        tok.type = TOKEN_OR;
      }
    }
    break;
  case '!':
    if (*(c + 1) == '=') {
      c++;
      tok.type = TOKEN_NOT_EQUALS;
    } else {
      tok.type = TOKEN_NOT;
    }
    break;
  case '\'':
  case '\"':
    return lex_string(l, c, start);
  default:
    tok.type = TOKEN_UNKNOWN;
  }

  c++;

  size_t len = c - start;

  tok.start = start;
  tok.length = len;
  tok.line_info = (LineInfo){l->line_offset, l->line, start - l->line_offset};

  l->pos = c - l->f->src;

  return tok;
}

Token lex_peek(Lexer *l) {
  if (!l->has_peeked) {
    l->peeked = lex_next(l);
    l->has_peeked = true;
  }
  return l->peeked;
}

b8 lex_expect_(Lexer *l, TokenType tok_type, Token *tok_actual) {
  // expect the next token without advancing
  *tok_actual = lex_peek(l);
  if ((*tok_actual).type == tok_type)
    return 1;

  return 0;
}

b8 lex_expect_range_(Lexer *l, TokenType start, TokenType end,
                     Token *tok_actual) {
  // expect the next token to be in an ordered range of types
  *tok_actual = lex_peek(l);
  if ((*tok_actual).type >= start && (*tok_actual).type <= end)
    return 1;

  return 0;
}

b8 lex_expect_next_(Lexer *l, TokenType tok_type, Token *tok_actual) {
  // expect the next token and consume it
  b8 val = lex_expect_(l, tok_type, tok_actual);
  lex_next(l);

  return val;
}

Token lex_number(Lexer *l, char *c, const char *start) {
  b8 is_float = false;

  LineInfo line_info =
      (LineInfo){l->line_offset, l->line, start - l->line_offset};

  while (isdigit(*c) || *c == '.') {
    if (*c++ == '.') {
      if (is_float == true) {
        error(l->f, line_info, "float cannot have multiple decimal points.");
      } else {
        is_float = true;
      }
    }
  }

  size_t len = c - start;
  l->pos = c - l->f->src;
  if (is_float)
    return (Token){start, len, line_info, TOKEN_FLOAT};

  return (Token){start, len, line_info, TOKEN_NUMBER};
}

Token lex_string(Lexer *l, char *c, const char *start) {
  char starting_char = *c;

  c++;

  while (*c != starting_char && *c != '\0') {
    c++;
  }

  LineInfo line_info =
      (LineInfo){l->line_offset, l->line, start - l->line_offset};

  if (*c == '\0') {
    error(l->f, line_info, "String literal is never terminated");
    exit(1);
  }

  size_t len = c - start - 1;
  c++;
  l->pos = c - l->f->src;
  return (Token){start + 1, len, line_info, TOKEN_STRING};
}

void lex_comment(Lexer *l, char *c) {
  c += 2;
  while (*c && *c != '\n')
    c++;
  l->pos = c - l->f->src;
}

Token lex_ident(Lexer *l, char *c, const char *start) {
  while (isalnum(*c) || *c == '_')
    ++c;

  size_t len = c - start;

#define MATCH(str) (len == sizeof(str) - 1 && strncmp(start, str, len) == 0)

  Token tok;

  if (MATCH("char_8"))
    tok.type = T_CHAR8;
  else if (MATCH("char_16"))
    tok.type = T_CHAR16;
  else if (MATCH("char_32"))
    tok.type = T_CHAR32;
  else if (MATCH("uint_8"))
    tok.type = T_UINT8;
  else if (MATCH("uint_16"))
    tok.type = T_UINT16;
  else if (MATCH("uint_32"))
    tok.type = T_UINT32;
  else if (MATCH("uint_64"))
    tok.type = T_UINT64;
  else if (MATCH("int_8"))
    tok.type = T_INT8;
  else if (MATCH("int_16"))
    tok.type = T_INT16;
  else if (MATCH("int_32"))
    tok.type = T_INT32;
  else if (MATCH("int_64"))
    tok.type = T_INT64;
  else if (MATCH("float_16"))
    tok.type = T_FLOAT16;
  else if (MATCH("float_32"))
    tok.type = T_FLOAT32;
  else if (MATCH("float_64"))
    tok.type = T_FLOAT64;
  else if (MATCH("bool"))
    tok.type = T_BOOL;
  else if (MATCH("struct"))
    tok.type = T_STRUCT;
  else if (MATCH("void"))
    tok.type = T_VOID;
  else if (MATCH("if"))
    tok.type = KW_IF;
  else if (MATCH("else"))
    tok.type = KW_ELSE;
  else if (MATCH("while"))
    tok.type = KW_WHILE;
  else if (MATCH("for"))
    tok.type = KW_FOR;
  else if (MATCH("switch"))
    tok.type = KW_SWITCH;
  else if (MATCH("case"))
    tok.type = KW_CASE;
  else if (MATCH("goto"))
    tok.type = KW_GOTO;
  else if (MATCH("return"))
    tok.type = KW_RETURN;
  else if (MATCH("true"))
    tok.type = KW_TRUE;
  else if (MATCH("false"))
    tok.type = KW_FALSE;
  else
    tok.type = TOKEN_IDENTIFIER;

#undef MATCH

  l->pos = c - l->f->src;

  tok.start = start;
  tok.length = len;
  tok.line_info = (LineInfo){l->line_offset, l->line, start - l->line_offset};
  return tok;
}

const char *token_type_to_cstr(TokenType t) {
  switch (t) {
  case T_CHAR8:
    return "CHAR8";
  case T_CHAR16:
    return "CHAR16";
  case T_CHAR32:
    return "CHAR32";

  case T_UINT8:
    return "UINT8";
  case T_UINT16:
    return "UINT16";
  case T_UINT32:
    return "UINT32";
  case T_UINT64:
    return "UINT64";

  case T_INT8:
    return "INT8";
  case T_INT16:
    return "INT16";
  case T_INT32:
    return "INT32";
  case T_INT64:
    return "INT64";

  case T_FLOAT16:
    return "FLOAT16";
  case T_FLOAT32:
    return "FLOAT32";
  case T_FLOAT64:
    return "FLOAT64";

  case T_BOOL:
    return "BOOL";

  case T_STRUCT:
    return "STRUCT";
  case T_VOID:
    return "VOID";

  case TOKEN_EQ:
    return "==";
  case TOKEN_ASSIGN:
    return "=";

  case TOKEN_PLUS:
    return "+";
  case TOKEN_PLUS_PLUS:
    return "++";
  case TOKEN_PLUS_ASSIGN:
    return "+=";

  case TOKEN_MINUS:
    return "-";
  case TOKEN_MINUS_MINUS:
    return "--";
  case TOKEN_MINUS_ASSIGN:
    return "-=";

  case TOKEN_ASTERISK:
    return "*";
  case TOKEN_ASTERISK_ASSIGN:
    return "*=";

  case TOKEN_SLASH:
    return "/";
  case TOKEN_SLASH_ASSIGN:
    return "/=";

  case TOKEN_MOD:
    return "%";
  case TOKEN_MOD_ASSIGN:
    return "%=";

  case TOKEN_XOR:
    return "^";
  case TOKEN_XOR_ASSIGN:
    return "^=";

  case TOKEN_COLON:
    return ":";
  case TOKEN_SEMICOLON:
    return ";";
  case TOKEN_COMMA:
    return ",";
  case TOKEN_LPAREN:
    return "(";
  case TOKEN_RPAREN:
    return ")";
  case TOKEN_LCURLY:
    return "{";
  case TOKEN_RCURLY:
    return "}";
  case TOKEN_LSQUARE:
    return "[";
  case TOKEN_RSQUARE:
    return "]";

  case TOKEN_LESS:
    return "<";
  case TOKEN_LESS_LESS:
    return "<<";
  case TOKEN_LESS_ASSIGN:
    return "<=";

  case TOKEN_GREATER:
    return ">";
  case TOKEN_GREATER_GREATER:
    return ">>";
  case TOKEN_GREATER_ASSIGN:
    return ">=";

  case TOKEN_AND:
    return "&";
  case TOKEN_AND_AND:
    return "&&";
  case TOKEN_AND_ASSIGN:
    return "&=";

  case TOKEN_OR:
    return "|";
  case TOKEN_OR_OR:
    return "||";
  case TOKEN_OR_ASSIGN:
    return "|=";

  case TOKEN_NOT:
    return "!";
  case TOKEN_TILDE:
    return "~";
  case TOKEN_NOT_EQUALS:
    return "!=";

  case KW_IF:
    return "IF";
  case KW_ELSE:
    return "ELSE";
  case KW_WHILE:
    return "WHILE";
  case KW_FOR:
    return "FOR";
  case KW_SWITCH:
    return "SWITCH";
  case KW_CASE:
    return "CASE";
  case KW_GOTO:
    return "GOTO";
  case KW_RETURN:
    return "RETURN";

  case TOKEN_IDENTIFIER:
    return "IDENTIFIER";
  case TOKEN_NUMBER:
    return "NUMBER";
  case TOKEN_FLOAT:
    return "FLOAT";
  case TOKEN_STRING:
    return "STRING";

  case TOKEN_EOF:
    return "EOF";
  case TOKEN_NEWLINE:
    return "\\n";
  case TOKEN_UNKNOWN:
  default:
    return "UNKNOWN";
  }
}

void token_print(Token t) {
  printf("Token: [line %zu:%zu] %-10s '%.*s' %zu\n", t.line_info.line,
         t.line_info.column, token_type_to_cstr(t.type), (int)t.length, t.start,
         t.length);
}
