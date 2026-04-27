#ifndef MODC_TYPES_H
#define MODC_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#include "ast.h"
#include "lexer.h"
#include "str.h"

typedef struct ModCType ModCType;
typedef struct Ast Ast;

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

  ModCType *ptr_to;

  union {
    TokenType builtin;

    struct {
      ModCType *base;
      int depth;
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

void modctype_memory_init(void);
void modctype_memory_release(void);

ModCType *type_to_builtin(TokenType type);
String *modctype_to_string(ModCType *type);

ModCType *make_pointer_type(ModCType *base, int ptr_depth);
ModCType *make_array_type(ModCType *base, int len);
ModCType *make_function_type(ModCType *ret_type, Ast **params, int param_count);
ModCType *signed_type_wider_than(size_t size);
ModCType *make_combined_type(Ast *node);

#endif // !MODC_TYPES_H
