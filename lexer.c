////////////////////////////////////////////////////////////////
// Token

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "k9cc.h"

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

Token *skip(Token *tok, const char *op) {
  if (!equal(tok, op)) {
    error_tok(tok, "expected '%s'", op);
  }
  return tok->next;
}

void dump_token_one(Token *tok) {
  char *s;
  switch (tok->kind) {
  case TK_RESERVED:
    s = strndup(tok->loc, tok->len);
    report("[TK_RESERVED] %s\n", s);
    free(s);
    break;
  case TK_NUM:
    report("[TK_NUM] %ld\n", tok->val);
    break;
  case TK_EOF:
    report("[TK_EOF]\n");
    break;
  default:
    report("unknown toknen kind\n");
  }
}
void dump_token(Token *tok) {
  report("\n** Token\n");
  for (; tok; tok = tok->next) {
    dump_token_one(tok);
  }
}

// tokenを作成してcurにつなげる
static Token *new_token(TokenKind kind, Token *cur, char *str, int len, int column) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = str;
  tok->len = len;
  tok->column = column;
  cur->next = tok;
  return tok;
}
// Tokenize p and returns new tokens.
Token *tokenize(char *src) {
  Token head = {};
  Token *cur = &head;
  int column = 0;

  current_input = src;
  while (*src) {
    if (isspace(*src)) {
      src++;
      continue;
    }

    column = src - current_input;
    // 数字
    if (isdigit(*src)) {
      cur = new_token(TK_NUM, cur, src, 0, column);
      char *p = src;
      cur->val = strtol(src, &src, 10);
      cur->len = src - p;
      continue;
    }

    // 記号
    static char *punctures[] = {
      "+", "-", "*", "/",
      "(", ")",
      "==", "!=",
      "<=", ">=",
      ">", "<",
      NULL
    };
    char **punc;
    for (punc = punctures; *punc; punc++) {
      size_t len = strlen(*punc);
      if (!strncmp(src, *punc, len)) {
        cur = new_token(TK_RESERVED, cur, src, len, column);
        src += len;
        break;
      }
    }
    if (!*punc) {
      error_at(src, "invalid punctures");
    }
  }
  new_token(TK_EOF, cur, src, 0, column);
  return head.next;
}
