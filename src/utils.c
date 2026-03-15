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

  fprintf(stderr, "modcc: error: line %zu: ", line);

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
  fflush(stderr);

  exit(1);
}

char *readin(char *file) {
  int fd;
  struct stat st;
  char *raw;

  if (strrchr(file, '.') == NULL)
    error(0, "file must end in '.modc'");

  if (!!strcmp(strrchr(file, '.'), ".modc"))
    error(0, "file must end in '.modc'");

  if ((fd = open(file, O_RDONLY)) == -1)
    error(0, "couldn't open %s", file);

  if (fstat(fd, &st) == -1)
    error(0, "couldn't get file size");

  if ((raw = malloc(st.st_size + 1)) == NULL)
    error(0, "malloc failed when reading file");

  if (read(fd, raw, st.st_size) != st.st_size)
    error(0, "couldn't read %s", file);
  raw[st.st_size] = '\0';

  close(fd);

  return raw;
}
