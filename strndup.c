// from
// https://stackoverflow.com/questions/46013382/c-strndup-implicit-declaration
#include "nacc.h"

char *strndup(const char *s, size_t n) {
  char *p = memchr(s, '\0', n);
  if (p != NULL) n = p - s;
  p = malloc(n + 1);
  if (p != NULL) {
    memcpy(p, s, n);
    p[n] = '\0';
  }
  return p;
}
