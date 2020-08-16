#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

////////////////////////////////////////////////////////////////
// code emit
void emit(const char *fmt, ...) {
  FILE *fpout = stdout;
  size_t len = strlen(fmt);

  if (*fmt != '.' && (0 < len && fmt[len - 1] != ':')) {
    // ラベルなどでない通常のコードのとき
    fprintf(fpout, "        ");
  }
  va_list ap;
  va_start(ap, fmt);
  vfprintf(fpout, fmt, ap);
  va_end(ap);
  fprintf(fpout, "\n");
}
void emit_head(void) {
  emit(".intel_syntax noprefix");
  emit(".globl main");
}

////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  if (argc != 2) {
    fputs("引数の個数が正しくありません\n", stderr);
    return 1;
  }
  char *p = argv[1];
  char *op;

  emit_head();
  emit("main:");

  emit("mov rax, %ld", strtol(p, &p, 10));
  while (*p) {
    switch (*p) {
    case '+':
      op = "add";
      break;
    case '-':
      op = "sub";
      break;
    default:
      fprintf(stderr, "予期しない文字です: %c\n", *p);
      return 1;
    }
    p++;
    emit("%s rax, %ld", op, strtol(p, &p, 10));
  }
  emit("ret");
  return 0;
}

// Local Variables:
// compile-command: "./dr make test"
// End:
