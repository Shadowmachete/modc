#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

void error(size_t line, const char *fmt, ...);
void warning(size_t line, const char *fmt, ...);
void info(size_t line, const char *fmt, ...);
void check_errors(void);
char *readin(char *file);

#endif // !UTILS_H
