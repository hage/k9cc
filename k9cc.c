#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "k9cc.h"

// 入力プログラム
char *user_input;

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
  }
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();
  codegen(node);
  return 0;
}
