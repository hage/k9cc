#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "k9cc.h"

////////////////////////////////////////////////////////////////
// emit code
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
// Token
char *current_input;

long get_number(Token *tok) {
  if (!tok || tok->kind != TK_NUM) {
    error_tok(tok, "数字が必要です");
  }
  return tok->val;
}

bool equal(Token *tok, const char *op) {
  if (!tok || tok->kind != TK_RESERVED) {
    return false;
  }
  return strlen(op) == tok->len && !strncmp(tok->loc, op, tok->len);
}

// tokenを作成してcurにつなげる
Token *new_token(TokenKind kind, Token *cur, char *str, int len, int column) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = str;
  tok->len = len;
  tok->column = column;
  cur->next = tok;
  return tok;
}
// Tokenize p and returns new tokens.
Token *tokenize(char *p) {
  Token head = {};
  Token *cur = &head;
  int column = 0;

  current_input = p;
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    column = p - current_input;
    // 数字
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0, column);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    // 記号
    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p, 1, column);
      p++;
      continue;
    }
    error_at(p, "invalid token");
  }
  new_token(TK_EOF, cur, p, 0, column);
  return head.next;
}

////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません\n");
  }
  Token *tok = tokenize(argv[1]);

  emit_head();
  emit("main:");

  // 最初のトークンは数字のはず
  emit("mov rax, %ld", get_number(tok));
  tok = tok->next;

  while (tok->kind != TK_EOF) {
    if (equal(tok, "+")) {
      emit("add rax, %ld", get_number(tok->next));
      tok = tok->next->next;
      continue;
    }
    if (equal(tok, "-")) {
      emit("sub rax, %ld", get_number(tok->next));
      tok = tok->next->next;
      continue;
    }
    error_tok(tok, "unknown token");
  }
  emit("ret");
  return 0;
}

// Local Variables:
// compile-command: "./dr make test"
// End:
