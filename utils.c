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

  fprintf(stderr, "custom compiler: error: line %zu: ", line);

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

static inline uint32_t murmur_32_scramble(uint32_t k) {
  k *= 0xcc9e2d51;
  k = (k << 15) | (k >> 17);
  k *= 0x1b873593;
  return k;
}

uint32_t murmur3_32(const uint8_t *key, size_t len, uint32_t seed) {
  // algorithm from wikipedia

  uint32_t h = seed;
  uint32_t k;
  /* Read in groups of 4. */
  for (size_t i = len >> 2; i; i--) {
    // Here is a source of differing results across endiannesses.
    // A swap here has no effects on hash properties though.
    memcpy(&k, key, sizeof(uint32_t));
    key += sizeof(uint32_t);
    h ^= murmur_32_scramble(k);
    h = (h << 13) | (h >> 19);
    h = h * 5 + 0xe6546b64;
  }
  /* Read the rest. */
  k = 0;
  for (size_t i = len & 3; i; i--) {
    k <<= 8;
    k |= key[i - 1];
  }
  // A swap is *not* necessary here because the preceding loop already
  // places the low bytes in the low places according to whatever endianness
  // we use. Swaps only apply when the memory is copied in a chunk.
  h ^= murmur_32_scramble(k);
  /* Finalize. */
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}
