#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "ast.h"
#include "lexer.h"
#include "modc_types.h"
#include "str.h"
#include "utils.h"

#define STR_FMT_UNWRAP(str) (int)(str)->len, (str)->data

static Arena modctype_arena;
static int modctype_arena_init = 0;

void modctype_memory_init(void) {
  if (!modctype_arena_init) {
    arena_init(&modctype_arena, sizeof(ModCType) * 32);
    modctype_arena_init = 1;
  }
}

void modctype_memory_release(void) {
  if (modctype_arena_init) {
    modctype_arena_init = 0;
    arena_clear(&modctype_arena);
  }
}

static ModCType *modctype_alloc(void) {
  return (ModCType *)arena_alloc(&modctype_arena, sizeof(ModCType));
}

ModCType *modctype_create(void) {
  ModCType *type = modctype_alloc();

  if (!type) {
    error(NULL, (LineInfo){0}, "arena failed to allocate modctype");
    exit(1);
  }

  memset(type, 0, sizeof(ModCType));

  return type;
}

ModCType **modctype_create_array(int len) {
  ModCType **modctype_array =
      (ModCType **)arena_alloc(&modctype_arena, sizeof(ModCType *) * len);

  if (!modctype_array) {
    error(NULL, (LineInfo){0}, "arena alloc failed creating ModCType array");
    exit(1);
  }

  memset(modctype_array, 0, sizeof(ModCType *) * len);

  return modctype_array;
}

ModCType *char_8_type = &(ModCType){
    .variant = TYPE_CHAR, .size = 1, .is_signed = false, .builtin = T_CHAR8};
ModCType *char_16_type = &(ModCType){
    .variant = TYPE_CHAR, .size = 2, .is_signed = false, .builtin = T_CHAR16};
ModCType *char_32_type = &(ModCType){
    .variant = TYPE_CHAR, .size = 4, .is_signed = false, .builtin = T_CHAR32};

ModCType *uint_8_type = &(ModCType){
    .variant = TYPE_INT, .size = 1, .is_signed = false, .builtin = T_UINT8};
ModCType *uint_16_type = &(ModCType){
    .variant = TYPE_INT, .size = 2, .is_signed = false, .builtin = T_UINT16};
ModCType *uint_32_type = &(ModCType){
    .variant = TYPE_INT, .size = 4, .is_signed = false, .builtin = T_UINT32};
ModCType *uint_64_type = &(ModCType){
    .variant = TYPE_INT, .size = 8, .is_signed = false, .builtin = T_UINT64};

ModCType *int_8_type = &(ModCType){
    .variant = TYPE_INT, .size = 1, .is_signed = true, .builtin = T_INT8};
ModCType *int_16_type = &(ModCType){
    .variant = TYPE_INT, .size = 2, .is_signed = true, .builtin = T_INT16};
ModCType *int_32_type = &(ModCType){
    .variant = TYPE_INT, .size = 4, .is_signed = true, .builtin = T_INT32};
ModCType *int_64_type = &(ModCType){
    .variant = TYPE_INT, .size = 8, .is_signed = true, .builtin = T_INT64};

ModCType *float_16_type = &(ModCType){
    .variant = TYPE_FLOAT, .size = 2, .is_signed = true, .builtin = T_FLOAT16};
ModCType *float_32_type = &(ModCType){
    .variant = TYPE_FLOAT, .size = 4, .is_signed = true, .builtin = T_FLOAT32};
ModCType *float_64_type = &(ModCType){
    .variant = TYPE_FLOAT, .size = 8, .is_signed = true, .builtin = T_FLOAT64};

ModCType *bool_type = &(ModCType){
    .variant = TYPE_BOOL, .size = 1, .is_signed = false, .builtin = T_BOOL};
ModCType *void_type = &(ModCType){
    .variant = TYPE_VOID, .size = 0, .is_signed = false, .builtin = T_VOID};
ModCType *error_type =
    &(ModCType){.variant = TYPE_ERROR, .size = 0, .is_signed = false};

ModCType *tok_type_to_modctype(TokenType type) {
  switch (type) {
  case T_CHAR8:
    return char_8_type;
  case T_CHAR16:
    return char_16_type;
  case T_CHAR32:
    return char_32_type;
  case T_UINT8:
    return uint_8_type;
  case T_UINT16:
    return uint_16_type;
  case T_UINT32:
    return uint_32_type;
  case T_UINT64:
    return uint_64_type;
  case T_INT8:
    return int_8_type;
  case T_INT16:
    return int_16_type;
  case T_INT32:
    return int_32_type;
  case T_INT64:
    return int_64_type;
  case T_FLOAT16:
    return float_16_type;
  case T_FLOAT32:
    return float_32_type;
  case T_FLOAT64:
    return float_64_type;
  case T_BOOL:
    return bool_type;
  case T_VOID:
    return void_type;
  default:
    error(NULL, (LineInfo){0},
          "Invalid builtin token %s passed to builtin_to_type",
          token_type_to_cstr(type));
  }
  return NULL;
}

String *modctype_to_string(ModCType *type) {
  switch (type->variant) {
  case TYPE_STRUCT:
    return str_dup_raw(type->structure.name, strlen(type->structure.name));
  case TYPE_ARRAY: {
    String *element_str = modctype_to_string(type->array.element);
    String *ret = str_new();
    str_catprintf(ret, "[%.*s; %d]", STR_FMT_UNWRAP(element_str),
                  type->array.length);
    return ret;
  }
  case TYPE_POINTER: {
    String *element_str = modctype_to_string(type->pointer.base);
    String *ret = str_new();
    str_catprintf(ret, "%.*s%*s", STR_FMT_UNWRAP(element_str),
                  type->pointer.depth, "*");
    return ret;
  }
  case TYPE_FUNCTION: {
    String *ret = str_new();
    str_putc(ret, '(');
    for (size_t i = 0; i < type->function.param_count; i++) {
      str_cat(ret, modctype_to_string(type->function.params[i]));
      if (i != type->function.param_count - 1)
        str_cat_raw(ret, ", ", 2);
    }
    str_cat_raw(ret, "; ", 2);
    str_cat(ret, modctype_to_string(type->function.return_type));
    str_putc(ret, ')');
    return ret;
  } break;
  default: {
    const char *cstr = token_type_to_cstr(type->builtin);
    return str_dup_raw(cstr, strlen(cstr));
  }
  }
}

ModCType *pointer_to(ModCType *base, int ptr_depth) {
  if (base->ptr_to)
    return base->ptr_to;

  ModCType *type = modctype_create();

  type->variant = TYPE_POINTER;
  type->size = 8;
  // assuming 64bit pointers, probably only going to support that
  type->pointer.base = base;
  type->pointer.depth = ptr_depth;

  base->ptr_to = type;
  return type;
}

ModCType *make_pointer_type(ModCType *base, int ptr_depth) {
  ModCType *type = base;
  for (int i = 0; i < ptr_depth; i++) {
    type = pointer_to(type, i + 1);
  }

  return type;
}

ModCType *make_array_type(ModCType *element, int len) {
  ModCType *type = modctype_create();

  type->variant = TYPE_ARRAY;
  type->size = element->size * len;
  type->array.element = element;
  type->array.length = len;

  return type;
}

ModCType *make_function_type(ModCType *ret_type, Ast **params,
                             int param_count) {
  ModCType *type = modctype_create();

  type->variant = TYPE_FUNCTION;
  type->size = 1;
  type->function.return_type = ret_type;

  ModCType **modctype_array = modctype_create_array(param_count);
  for (int i = 0; i < param_count; i++) {
    modctype_array[i] = params[i]->var_decl.type;
  }

  type->function.params = modctype_array;

  type->function.param_count = param_count;

  return type;
}

ModCType *signed_type_wider_than(size_t size) {
  switch (size) {
  case 1:
    return int_16_type;
  case 2:
    return int_32_type;
  case 4:
  default:
    return int_64_type;
  }
}

b8 t_is_numeric(ModCType *t) {
  return t->variant == TYPE_INT || t->variant == TYPE_FLOAT ||
         t->variant == TYPE_BOOL || t->variant == TYPE_POINTER;
}

ModCType *common_numeric_type(ModCType *a, ModCType *b, int *cast_lhs,
                              int *cast_rhs) {
  *cast_lhs = 0;
  *cast_rhs = 0;

  if (a == b)
    return a;

  // check if non-numeric
  if (!t_is_numeric(a) || !t_is_numeric(b))
    return NULL;

  b8 a_is_float = (a->variant == TYPE_FLOAT);
  b8 b_is_float = (b->variant == TYPE_FLOAT);

  ModCType *result;

  if (a_is_float != b_is_float) {
    ModCType *float_t = a_is_float ? a : b;
    ModCType *int_t = a_is_float ? b : a;

    if (int_t->size >= 8 && float_t->size <= 4)
      result = float_64_type;
    else if (int_t->size >= 4 && float_t->size == 2)
      result = float_32_type;
    else
      result = float_t;
  } else if (a_is_float) {
    result = (a->size >= b->size) ? a : b;
  } else {
    if (a->size != b->size) {
      result = (a->size >= b->size) ? a : b;
    } else {
      if (a->is_signed != b->is_signed) {
        result = signed_type_wider_than(a->size);
      } else {
        result = a;
      }
    }
  }

  *cast_lhs = (a != result);
  *cast_rhs = (b != result);

  return result;
}

ModCType *make_combined_type(Ast *node) {
  ModCType *left = node->binary.left->type;
  ModCType *right = node->binary.right->type;

  int cast_lhs, cast_rhs;

  ModCType *result = common_numeric_type(left, right, &cast_lhs, &cast_rhs);

  if (result == NULL) {
    return NULL;
  }

  if (cast_lhs) {
    LineInfo line_info = node->binary.left->line_info;
    node->binary.left = ast_cast(node->binary.left, result);
    node->binary.left->line_info = line_info;
  }
  if (cast_rhs) {
    LineInfo line_info = node->binary.right->line_info;
    node->binary.right = ast_cast(node->binary.right, result);
    node->binary.right->line_info = line_info;
  }

  return result;
}

b8 types_can_fit(ModCType *type_a, ModCType *type_b) {
  return type_a == type_b ||
         (type_a->variant == TYPE_INT && type_b->variant == TYPE_INT) ||
         (type_a->variant == TYPE_FLOAT && type_b->variant == TYPE_FLOAT) ||
         (type_a->variant == TYPE_CHAR && type_b->variant == TYPE_CHAR);
}
