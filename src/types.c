#include "types.h"
#include "lexer.h"

Type *char_8_type = &(Type){
    .variant = TYPE_CHAR, .size = 1, .is_signed = false, .builtin = T_CHAR8};
Type *char_16_type = &(Type){
    .variant = TYPE_CHAR, .size = 2, .is_signed = false, .builtin = T_CHAR16};
Type *char_32_type = &(Type){
    .variant = TYPE_CHAR, .size = 4, .is_signed = false, .builtin = T_CHAR32};

Type *uint_8_type = &(Type){
    .variant = TYPE_INT, .size = 1, .is_signed = false, .builtin = T_UINT8};
Type *uint_16_type = &(Type){
    .variant = TYPE_INT, .size = 2, .is_signed = false, .builtin = T_UINT16};
Type *uint_32_type = &(Type){
    .variant = TYPE_INT, .size = 4, .is_signed = false, .builtin = T_UINT32};
Type *uint_64_type = &(Type){
    .variant = TYPE_INT, .size = 8, .is_signed = false, .builtin = T_UINT64};

Type *int_8_type = &(Type){
    .variant = TYPE_INT, .size = 1, .is_signed = true, .builtin = T_INT8};
Type *int_16_type = &(Type){
    .variant = TYPE_INT, .size = 2, .is_signed = true, .builtin = T_INT16};
Type *int_32_type = &(Type){
    .variant = TYPE_INT, .size = 4, .is_signed = true, .builtin = T_INT32};
Type *int_64_type = &(Type){
    .variant = TYPE_INT, .size = 8, .is_signed = true, .builtin = T_INT64};

Type *float_16_type = &(Type){
    .variant = TYPE_FLOAT, .size = 2, .is_signed = false, .builtin = T_FLOAT16};
Type *float_32_type = &(Type){
    .variant = TYPE_FLOAT, .size = 4, .is_signed = false, .builtin = T_FLOAT32};
Type *float_64_type = &(Type){
    .variant = TYPE_FLOAT, .size = 8, .is_signed = false, .builtin = T_FLOAT64};

Type *bool_type = &(Type){
    .variant = TYPE_INT, .size = 1, .is_signed = false, .builtin = T_BOOL};
Type *void_type = &(Type){
    .variant = TYPE_INT, .size = 0, .is_signed = false, .builtin = T_VOID};
