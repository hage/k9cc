#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "k9cc.h"

// 入力プログラム
char *user_input;
static char source_buffer[4096]; // ソースを保持するバッファ

// k9cc -e <code>
// stdout | k9cc
// k9cc <sourcefile>
// 最後の起動式のときはsuffixが".c"でなければならず、
// また出力は<basename>.sになる。

int main(int argc, char **argv) {
  FILE *fpout = stdout;

  if (argc == 3) {
    // コマンドラインから
    if (!strcmp(argv[1], "-e")) {
      user_input = argv[2];
    }
    else {
      error("usage: k9cc -e <code>");
    }
  }
  else if (argc == 2) {
    // ファイルから
    char *fname = argv[1];
    size_t len = strlen(fname);
    char *suffix = (argv[1] + len - 2);
    if (2 <= len && !strcmp(suffix, ".c")) {
      FILE *fpin = fopen(fname, "r");
      if (!fpin) {
        error("ファイル %s が見つかりません", fname);
      }
      fread(source_buffer, 1, sizeof(source_buffer), fpin);
      fclose(fpin);

      user_input = source_buffer;
      suffix[1] = 's';
      fpout = fopen(fname, "w");
    }
    else {
      error("%s はCのソースではありません", fname);
    }
  }
  else if (argc == 1) {
    // 標準入力から
    fread(source_buffer, 1, sizeof(source_buffer), stdin);
    user_input = source_buffer;
  }
  else {
    error("引数の個数が正しくありません");
  }
  token = tokenize(user_input);
  Code *code = program();
  codegen(code, fpout);
  return 0;
}
