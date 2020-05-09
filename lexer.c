#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "k9cc.h"

// 現在着目しているトークン
Token *token;

static bool is_1st_letter_of_symbol(int c) {
  return c == '_' || isalpha(c);
}

static bool is_letter_of_symbol(int c) {
  return is_1st_letter_of_symbol(c) || isdigit(c);
}

/* トークンが期待している記号のときは
 * トークンを1つ読み進めて真を返す。
 * それ以外の場合は偽を返す */
bool consume(char *op) {
  if (token->kind != TK_RESERVED
      || strlen(op) != token->len
      || memcmp(token->str, op, token->len)) {
    return false;
  }
  token = token->next;
  return true;
}

// 期待したトークンの型のときは
// トークンを1つ読み進めてそのトークンを返す。
// それ以外のときはNULLを返する
Token *consume_kind(TokenKind kind) {
  if (token->kind == kind) {
    Token *r = token;
    token = token->next;
    return r;
  }
  return NULL;
}

// トークンが変数のときには
// トークンを1つ読み進めて返す。
// それ以外のときはNULLを返す。
Token *consume_ident() {
  if (token->kind == TK_IDENT) {
    Token *r = token;
    token = token->next;
    return r;
  }
  return NULL;
}

/* 次のトークンが期待している記号のときには、トークンを1つ読みすすめる。
 * それ以外のときはエラーを報告する */
void expect(char *op) {
  if (token->kind != TK_RESERVED
      || token->len != strlen(op)
      || memcmp(token->str, op, token->len)) {
    error_at(token->str, "'%c'ではありません", op);
  }
  token = token->next;
}

/* 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
 * それ以外のときはエラーを報告する。 */
int expect_number() {
  if (token->kind != TK_NUM) {
    error_at(token->str, "数ではありません");
  }
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurにつなげる
static Token *new_token(TokenKind kind, Token *cur, char *str, size_t len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

static bool tokenize_keyword(const char *keyword, TokenKind kind, Token **pcur, char **pc) {
  size_t len = strlen(keyword);
  if (!strncmp(*pc, keyword, len) && !is_letter_of_symbol((*pc)[len])) {
    *pcur = new_token(kind, *pcur, *pc, 0);
    *pc += len;
    return true;
  }
  return false;
}


// 入力文字列pをトークナイズして返す
Token *tokenize(char *p) {
  static const char *operators[] = {
    "==", "!=", "<=", ">=",
    "<", ">",
    "-", "+", "/", "*", "(", ")",
    "=", ";",
    NULL
  };
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    // return
    if (tokenize_keyword("return", TK_RETURN, &cur, &p)) {
      continue;
    }
    else if(tokenize_keyword("if", TK_IF, &cur, &p)) {
      continue;
    }
    else if(tokenize_keyword("else", TK_ELSE, &cur, &p)) {
      continue;
    }
    else if(tokenize_keyword("while", TK_WHILE, &cur, &p)) {
      continue;
    }

    // 変数
    if (is_1st_letter_of_symbol(*p)) {
      char *q = p;
      for (; *p && is_letter_of_symbol(*p); p++)
        ;
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }

    // 演算子
    char const **op;
    for (op = operators; *op; op++) {
      size_t len = strlen(*op);
      if (len <= strlen(p) && !memcmp(p, *op, len)) {
        cur = new_token(TK_RESERVED, cur, p, len);
        p += len;
        break;
      }
    }
    // *opがNULLでないときは演算子が見つかったのでループを回す
    if (*op) {
      continue;
    }

    // 数字
    if (isdigit(*p)) {
      char *org_p = p;
      int val = strtol(p, &p, 10);
      cur = new_token(TK_NUM, cur, p, org_p - p);
      cur->val = val;
      continue;
    }
    error_at(p, "トークナイズできません");
  }
  new_token(TK_EOF, cur, p, 0);
  return head.next;
}
