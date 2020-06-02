#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "k9cc.h"

void warn(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
}

// エラーを報告する関数
// printfと同じ引数を取る
void pp(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

void error_at(const char *loc, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static void v_print_at_by_where(TokWhere where, const char *fmt, va_list ap) {
  fprintf(stderr, "line: %d, column: %d\n", where.line, where.column);
  for (char *p = where.beg_line; *p && *p != '\n'; p++) {
    fputc(*p, stderr);
  }
  fprintf(stderr, "\n%*s", where.column, ""); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

void error_at_by_where(TokWhere where, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  v_print_at_by_where(where, fmt, ap);
  va_end(ap);
  exit(1);
}
void error_at_by_token(Token *tok, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  v_print_at_by_where(tok->where, fmt, ap);
  va_end(ap);
  exit(1);

}
