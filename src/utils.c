#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

void error(size_t line, const char *fmt, ...) {
  va_list ap;

  fprintf(stderr, "modcc: [ERROR]: line %zu: ", line);

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
  fflush(stderr);
}

void warning(size_t line, const char *fmt, ...) {
  va_list ap;

  fprintf(stdout, "modcc: [WARNING]: line %zu: ", line);

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);

  fputc('\n', stdout);
  fflush(stdout);
}

void info(size_t line, const char *fmt, ...) {
  va_list ap;

  fprintf(stdout, "modcc: [INFO]: line %zu: ", line);

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);

  fputc('\n', stdout);
  fflush(stdout);
}

char *readin(char *file) {
  int fd;
  struct stat st;
  char *raw;

  int _errored = 0;

  if (strrchr(file, '.') == NULL) {
    error(0, "file must end in '.modc'");
    _errored = 1;
  }

  if (!!strcmp(strrchr(file, '.'), ".modc")) {
    error(0, "file must end in '.modc'");
    _errored = 1;
  }

  if ((fd = open(file, O_RDONLY)) == -1) {
    error(0, "couldn't open %s", file);
    _errored = 1;
  }

  if (fstat(fd, &st) == -1) {
    error(0, "couldn't get file size");
    _errored = 1;
  }

  if ((raw = malloc(st.st_size + 1)) == NULL) {
    error(0, "malloc failed when reading file");
    _errored = 1;
  }

  if (read(fd, raw, st.st_size) != st.st_size) {
    error(0, "couldn't read %s", file);
    _errored = 1;
  }

  raw[st.st_size] = '\0';

  close(fd);

  if (_errored) {
    free(raw);
    exit(1);
  }

  return raw;
}
