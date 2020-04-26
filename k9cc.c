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

void eprintf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    eprintf("引数の個数が正しくありません\n");
    return 1;
  }

  print_header();

  char *p = argv[1];

  cprintf("mov rax, %ld", strtol(p, &p, 10));
  while (*p) {
    if (*p == '+') {
      p++;
      cprintf("add rax, %ld", strtol(p, &p, 10));
      continue;
    }

    if (*p == '-') {
      p++;
      cprintf("sub rax, %ld", strtol(p, &p, 10));
      continue;
    }

    eprintf("予期しない文字です: '%c'\n", *p);
    return 1;
  }
  cprintf("ret");

  return 0;
}
