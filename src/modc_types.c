#include "modc_types.h"
#include "arena.h"
#include "lexer.h"
#include "utils.h"
#include <string.h>

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

  if (!type)
    error(0, "malloc failed creating modctype");

  memset(type, 0, sizeof(ModCType));

  return type;
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
    .variant = TYPE_FLOAT, .size = 2, .is_signed = false, .builtin = T_FLOAT16};
ModCType *float_32_type = &(ModCType){
    .variant = TYPE_FLOAT, .size = 4, .is_signed = false, .builtin = T_FLOAT32};
ModCType *float_64_type = &(ModCType){
    .variant = TYPE_FLOAT, .size = 8, .is_signed = false, .builtin = T_FLOAT64};

ModCType *bool_type = &(ModCType){
    .variant = TYPE_INT, .size = 1, .is_signed = false, .builtin = T_BOOL};
ModCType *void_type = &(ModCType){
    .variant = TYPE_INT, .size = 0, .is_signed = false, .builtin = T_VOID};

ModCType *type_to_builtin(TokenType type) {
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
    error(0, "Invalid builtin type %s passed to builtin_to_type",
          token_type_to_string(type));
  }
  return NULL;
}

ModCType *pointer_to(ModCType *base) {
  if (base->ptr_to)
    return base->ptr_to;

  ModCType *type = modctype_create();

  type->variant = TYPE_POINTER;
  type->size = 8;
  type->pointer.base = base;

  base->ptr_to = type;
  return type;
}

ModCType *make_pointer_type(ModCType *base, int ptr_depth) {
  ModCType *type = base;
  for (int i = 0; i < ptr_depth; i++) {
    type = pointer_to(type);
  }

  return type;
}
