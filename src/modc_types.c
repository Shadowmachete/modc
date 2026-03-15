#include "modc_types.h"

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
