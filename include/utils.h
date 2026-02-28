#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

void error(size_t line, const char *fmt, ...);
char *readin(char *file);

#endif // !UTILS_H
