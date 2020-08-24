#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "k9cc.h"

// report an error and abnormal exit
void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputs("\n", stderr);
  exit(1);
}

void verror_at(const char *fmt, int pos, va_list ap) {
  fprintf(stderr, "%s\n", current_input);
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_tok(Token *tok, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  verror_at(fmt, tok->column, ap);
  va_end(ap);
  exit(1);
}

void error_at(char *pos, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  verror_at(fmt, pos - current_input, ap);
  va_end(ap);
  exit(1);
}

void va_report(const char *fmt, va_list ap) {
  vfprintf(stderr, fmt, ap);
}

void report(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  va_report(fmt, ap);
  va_end(ap);
}
