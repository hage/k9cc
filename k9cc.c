#include <stdio.h>
#include <stdlib.h>

void print_header(void) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  print_header();

  printf("        mov rax, %d\n", atoi(argv[1]));
  printf("        ret\n");
  return 0;
}
