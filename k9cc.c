#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void print_header(void) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

void cprintf(const char *fmt, ...) {
  printf("        ");
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  print_header();

  cprintf("mov rax, %d", atoi(argv[1]));
  cprintf("ret");
  return 0;
}
