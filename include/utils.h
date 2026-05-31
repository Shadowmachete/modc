#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

typedef struct File File;
typedef struct LineInfo LineInfo;

void error(File *f, LineInfo li, const char *fmt, ...);
void warning(File *f, LineInfo li, const char *fmt, ...);
void info(File *f, LineInfo li, const char *fmt, ...);
void check_errors(void);
File readin(const char *file);

struct File {
  const char *filename;
  const char *src;
};

struct LineInfo {
  const char *line_offset;
  size_t line;
  size_t column;
};

#endif // !UTILS_H
