#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

void error(size_t line, const char *fmt, ...);
char *readin(char *file);
uint32_t murmur3_32(const uint8_t *key, size_t len, uint32_t seed);

#endif
