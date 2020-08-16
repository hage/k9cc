#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

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
// report
void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(1);
}

////////////////////////////////////////////////////////////////
// Token
typedef enum {
  TK_RESERVED,                  // Keywords or punctuators
  TK_NUM,                       // Numeric literals
  TK_EOF,                       // End-of-file markers
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  long val;                     // kindがTK_NUMだったときその値
  char *loc;                    // Token location
  int len;                      // Token length
};


long get_number(Token *tok) {
  if (!tok || tok->kind != TK_NUM) {
    error("数字が必要です");
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
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}
// Tokenize p and returns new tokens.
Token *tokenize(char *p) {
  Token head = {};
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    // 数字
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    // 記号
    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }
    error("invalid token: %s", p);
  }
  new_token(TK_EOF, cur, p, 0);
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
    error("unknown token: %s", tok->loc);
  }
  emit("ret");
  return 0;
}

// Local Variables:
// compile-command: "./dr make test"
// End:
