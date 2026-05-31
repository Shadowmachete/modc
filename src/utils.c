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

void pretty_print_diagnostic(FILE *out, LineInfo li) {
  char *line_end = strchr(li.line_offset, '\n');
  size_t line_length = line_end - li.line_offset;

  fprintf(out, "\t%.*s\n\t%*s^\n", (int)line_length, li.line_offset,
          (int)li.column, " ");
}

void error(File *f, LineInfo li, const char *fmt, ...) {
  va_list ap;

  if (f != NULL && li.line != 0) {
    fprintf(stderr, "%s:%zu,%zu: [ERROR]: ", f->filename, li.line, li.column);
  } else {
    fprintf(stderr, "[ERROR]: line %zu: ", li.line);
  }

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
  if (f != NULL && li.line != 0)
    pretty_print_diagnostic(stderr, li);

  fflush(stderr);

  num_errors++;
}

void warning(File *f, LineInfo li, const char *fmt, ...) {
  va_list ap;

  if (f != NULL && li.line != 0) {
    fprintf(stdout, "%s:%zu,%zu: [WARNING]: ", f->filename, li.line, li.column);
  } else {
    fprintf(stdout, "[WARNING]: line %zu: ", li.line);
  }

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);

  fputc('\n', stdout);

  if (f != NULL && li.line != 0)
    pretty_print_diagnostic(stdout, li);

  fflush(stdout);

  num_warnings++;
}

void info(File *f, LineInfo li, const char *fmt, ...) {
  va_list ap;

  if (f != NULL && li.line != 0) {
    fprintf(stdout, "%s:%zu,%zu: [INFO]: ", f->filename, li.line, li.column);
  } else {
    fprintf(stdout, "[INFO]: line %zu: ", li.line);
  }

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);

  fputc('\n', stdout);

  if (f != NULL && li.line != 0)
    pretty_print_diagnostic(stdout, li);

  fflush(stdout);
}

void check_errors(void) {
  if (num_errors > 0 || num_warnings > 0) {
    fprintf(stderr,
            "modcc: compilation failed with %d error(s) and %d warning(s)\n",
            num_errors, num_warnings);
    exit(1);
  }
}

File readin(const char *file) {
  int fd;
  struct stat st;
  char *raw;

  if (strrchr(file, '.') == NULL) {
    error(NULL, (LineInfo){0}, "file %s must end in '.modc'", file);
    exit(1);
  }

  if (!!strcmp(strrchr(file, '.'), ".modc")) {
    error(NULL, (LineInfo){0}, "file %s must end in '.modc'", file);
    exit(1);
  }

  if ((fd = open(file, O_RDONLY)) == -1) {
    error(NULL, (LineInfo){0}, "couldn't open %s", file);
    exit(1);
  }

  if (fstat(fd, &st) == -1) {
    error(NULL, (LineInfo){0}, "couldn't get file size for %s", file);
    close(fd);
    exit(1);
  }

  if ((raw = malloc(st.st_size + 1)) == NULL) {
    error(NULL, (LineInfo){0}, "malloc failed when reading file %s", file);
    close(fd);
    exit(1);
  }

  if (read(fd, raw, st.st_size) != st.st_size) {
    error(NULL, (LineInfo){0}, "couldn't read %s", file);
    free(raw);
    close(fd);
    exit(1);
  }

  raw[st.st_size] = '\0';
  close(fd);

  return (File){file, raw};
}
