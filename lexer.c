////////////////////////////////////////////////////////////////
// Token

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "k9cc.h"

char *current_input;

// 名前に使える1文字目
static int is_nameletter1(char c) {
  return isalpha(c) || c == '_';
}

// 名前に使える2文字目以降
static int is_nameletter2(char c) {
  return is_nameletter1(c) || isdigit(c);
}

// identを切り出して返す
char *identdup(Token *tok) {
  if (tok->kind != TK_IDENT) {
    error_tok(tok, "token type is not TK_IDENT");
  }
  return strndup(tok->loc, tok->len);
}

long get_number(Token *tok) {
  if (!tok || tok->kind != TK_NUM) {
    error_tok(tok, "lexser/数字が必要です");
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
    error_tok(tok, "lexser/expected '%s'", op);
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
  case TK_IDENT:
    s = strndup(tok->loc, tok->len);
    report("[TK_IDENT] %s\n", s);
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
static Token *new_token(TokenKind kind, Token *cur, const char *str, int len, int column) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = str;
  tok->len = len;
  tok->column = column;
  cur->next = tok;
  return tok;
}

// srcからkeywordが見つかったらトークンのチェーンにつなげてtrueを返す
static size_t keyword(Token **pcur, char **psrc, const char *keyword, int column) {
  size_t len = startswith(*psrc, keyword);
  if (len && !is_nameletter2((*psrc)[len])) {
    *pcur = new_token(TK_RESERVED, *pcur, *psrc, len, column);
    *psrc += len;
    return len;
  }
  return 0;
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

    // Multi-letter punctuators
    static char *punctuators[] = {
      "==", "!=",
      "<=", ">=",
      NULL
    };
    char **punc;
    for (punc = punctuators; *punc; punc++) {
      size_t len = strlen(*punc);
      if (!strncmp(src, *punc, len)) {
        cur = new_token(TK_RESERVED, cur, src, len, column);
        src += len;
        break;
      }
    }
    if (*punc) {
      continue;
    }

    // Single-letter punctuators
    if (ispunct(*src)) {
      cur = new_token(TK_RESERVED, cur, src++, 1, column);
      continue;
    }

    static char *keywords[] = {
      "return", "if", "else", "while", "for",
      NULL
    };
    char **kwd = keywords;
    for (; *kwd; kwd++) {
      if (keyword(&cur, &src, *kwd, column)) {
        break;
      }
    }
    if (*kwd) {
      continue;
    }

    // Identifire
    if (isalpha(*src)) {
      char *p;
      for (p = src + 1; *p && is_nameletter2(*p); p++)
        ;
      cur = new_token(TK_IDENT, cur, src, p - src, column);
      src = p;
      continue;
    }

    error_at(src, "lexser/invalid token");

  }
  new_token(TK_EOF, cur, src, 0, column);
  return head.next;
}
