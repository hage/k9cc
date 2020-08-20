#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "k9cc.h"

////////////////////////////////////////////////////////////////
// emit code

////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません\n");
  }
  Token *tok = tokenize(argv[1]), *toktop = tok;
  Node *node = parse(tok);

  // dump_token(toktop);
  // walk(node);

  codegen(node);
  return 0;
}

// Local Variables:
// compile-command: "./dr make test"
// End:
