#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

static int num_errors = 0;
static int num_warnings = 0;

void error(size_t line, const char *fmt, ...) {
  va_list ap;

  fprintf(stderr, "modcc: [ERROR]: line %zu: ", line);

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
  fflush(stderr);

  num_errors++;
}

void warning(size_t line, const char *fmt, ...) {
  va_list ap;

  fprintf(stdout, "modcc: [WARNING]: line %zu: ", line);

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);

  fputc('\n', stdout);
  fflush(stdout);

  num_warnings++;
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

void check_errors(void) {
  if (num_errors > 0) {
    fprintf(stderr,
            "modcc: compilation failed with %d error(s) and %d warning(s)\n",
            num_errors, num_warnings);
    exit(1);
  }
}

char *readin(char *file) {
  int fd;
  struct stat st;
  char *raw;

  if (strrchr(file, '.') == NULL) {
    error(0, "file must end in '.modc'");
    exit(1);
  }

  if (!!strcmp(strrchr(file, '.'), ".modc")) {
    error(0, "file must end in '.modc'");
    exit(1);
  }

  if ((fd = open(file, O_RDONLY)) == -1) {
    error(0, "couldn't open %s", file);
    exit(1);
  }

  if (fstat(fd, &st) == -1) {
    error(0, "couldn't get file size");
    close(fd);
    exit(1);
  }

  if ((raw = malloc(st.st_size + 1)) == NULL) {
    error(0, "malloc failed when reading file");
    close(fd);
    exit(1);
  }

  if (read(fd, raw, st.st_size) != st.st_size) {
    error(0, "couldn't read %s", file);
    free(raw);
    close(fd);
    exit(1);
  }

  raw[st.st_size] = '\0';
  close(fd);

  return raw;
}
