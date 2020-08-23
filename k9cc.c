////////////////////////////////////////////////////////////////
// K9 C Compiler

#include "k9cc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません\n");
  }
  Token *tok = tokenize(argv[1]), *toktop = tok;
  Node *node = parse(tok);

  // dump_token(toktop); walk(node);

  codegen(node);
  return 0;
}
