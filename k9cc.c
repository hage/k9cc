#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "k9cc.h"

// 入力プログラム
char *user_input;
static char source_buffer[4096]; // ソースを保持するバッファ
char *source_file;

static char *mystrdup(const char *p) {
  return strcpy(malloc(strlen(p) + 1), p);
}

// k9cc -e <code>
// stdout | k9cc
// k9cc <sourcefile>
// 最後の起動式のときはsuffixが".c"でなければならず、
// また出力は<basename>.sになる。

int main(int argc, char **argv) {
  FILE *fpout = stdout;

  if (argc == 3) {
    // コマンドラインから
    source_file = "<COMMAND-LINE>";
    if (!strcmp(argv[1], "-e")) {
      user_input = argv[2];
    }
    else {
      error("usage: k9cc -e <code>");
    }
  }
  else if (argc == 2) {
    // ファイルから
    source_file = argv[1];

    char *out_file = mystrdup(source_file);
    char *suffix = strrchr(out_file, '.');

    if (suffix && !strcmp(suffix, ".c")) {
      FILE *fpin = fopen(source_file, "r");
      if (!fpin) {
        error("ファイル %s が見つかりません", source_file);
      }
      fread(source_buffer, 1, sizeof(source_buffer), fpin);
      fclose(fpin);

      user_input = source_buffer;
      suffix[1] = 's';
      fpout = fopen(out_file, "w");
    }
    else {
      error("%s はCのソースではありません", source_file);
    }
  }
  else if (argc == 1) {
    // 標準入力から
    source_file = "<STDIN>";
    fread(source_buffer, 1, sizeof(source_buffer), stdin);
    user_input = source_buffer;
  }
  else {
    error("引数の個数が正しくありません");
  }
  token = tokenize(user_input);
  Funcdef *fdef = program();
  codegen(fdef, fpout);
  return 0;
}
