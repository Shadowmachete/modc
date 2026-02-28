#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stddef.h>

#include "lexer.h"

typedef struct Type Type;

struct Type {
  enum {
    TYPE_VOID,
    TYPE_BOOL,

    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,

    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_STRUCT,
  } variant;

  size_t size;
  size_t alignment;
  bool is_signed;

  union {
    TokenType builtin;

    struct {
      Type *base;
    } pointer;

    struct {
      Type *element;
      size_t length;
    } array;

    struct {
      Type *return_type;
      Type **params;
      size_t param_count;
    } function;

    struct {
      const char *name;
      Type **field_types;
      const char **field_names;
      size_t field_count;
    } structure;
  };
};

extern Type *char_8_type;
extern Type *char_16_type;
extern Type *char_32_type;
extern Type *uint_8_type;
extern Type *uint_16_type;
extern Type *uint_32_type;
extern Type *uint_64_type;
extern Type *int_8_type;
extern Type *int_16_type;
extern Type *int_32_type;
extern Type *int_64_type;
extern Type *float_16_type;
extern Type *float_32_type;
extern Type *float_64_type;
extern Type *bool_type;
extern Type *void_type;

#endif // !TYPES_H
