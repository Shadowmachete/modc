#ifndef MODC_TYPES_H
#define MODC_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#include "lexer.h"

typedef struct ModCType ModCType;

struct ModCType {
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
      ModCType *base;
    } pointer;

    struct {
      ModCType *element;
      size_t length;
    } array;

    struct {
      ModCType *return_type;
      ModCType **params;
      size_t param_count;
    } function;

    struct {
      const char *name;
      ModCType **field_types;
      const char **field_names;
      size_t field_count;
    } structure;
  };
};

extern ModCType *char_8_type;
extern ModCType *char_16_type;
extern ModCType *char_32_type;
extern ModCType *uint_8_type;
extern ModCType *uint_16_type;
extern ModCType *uint_32_type;
extern ModCType *uint_64_type;
extern ModCType *int_8_type;
extern ModCType *int_16_type;
extern ModCType *int_32_type;
extern ModCType *int_64_type;
extern ModCType *float_16_type;
extern ModCType *float_32_type;
extern ModCType *float_64_type;
extern ModCType *bool_type;
extern ModCType *void_type;

#endif // !TYPES_H
