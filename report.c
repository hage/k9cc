#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "k9cc.h"

void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputs("\n", stderr);
  exit(1);
}
