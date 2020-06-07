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

const char *tokstrdup(Token *tok) {
  char *str = malloc(tok->len + 1);
  memcpy(str, tok->str, tok->len);
  str[tok->len] = '\0';
  return str;
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
    error_at_by_token(token, "'%s'ではありません", op);
  }
  token = token->next;
}

/* 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
 * それ以外のときはエラーを報告する。 */
int expect_number() {
  if (token->kind != TK_NUM) {
    error_at_by_token(token, "数ではありません");
  }
  int val = token->val;
  token = token->next;
  return val;
}

// 次のトークンがopとkindに合致している場合にtrueを返し1つ読みすすめる。
// opにNULLを渡したとき、kindにTK_NONEを渡したときそれぞれを無視する。
// マッチしなくてもエラーにはならない。
bool consume_if_matched(char *op, TokenKind kind) {
  bool kind_match = kind == TK_NONE || kind == token->kind;
  if (!op) {
    token = token->next;
    return kind_match;
  }
  else {
    size_t len = strlen(op);
    if (kind_match && len == token->len && memcmp(op, token->str, len)) {
      token = token->next;
      return true;
    }
    return false;
  }
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurにつなげる
static Token *new_token(TokenKind kind, Token *cur, char *str, size_t len, TokWhere *where) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  tok->where = *where;
  tok->where.column += len;
  cur->next = tok;
  return tok;
}

static bool tokenize_keyword(const char *keyword, TokenKind kind, Token **pcur, char **pc, TokWhere *where) {
  size_t len = strlen(keyword);
  if (!strncmp(*pc, keyword, len) && !is_letter_of_symbol((*pc)[len])) {
    *pcur = new_token(kind, *pcur, *pc, 0, where);
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
    "&", "=", ";",
    "{", "}", ",",
    NULL
  };
  Token head;
  head.next = NULL;
  Token *cur = &head;

  TokWhere where = {p, 1, 1};
  where.beg_line = p;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      if (*p == '\n') {
        where.beg_line = p + 1;
        where.line++;
        where.column = 1;
      }
      else {
	where.column++;
      }
      p++;
      continue;
    }

    // return
    if (tokenize_keyword("return", TK_RETURN, &cur, &p, &where)) {
      continue;
    }
    else if (tokenize_keyword("if", TK_IF, &cur, &p, &where)) {
      continue;
    }
    else if (tokenize_keyword("else", TK_ELSE, &cur, &p, &where)) {
      continue;
    }
    else if (tokenize_keyword("while", TK_WHILE, &cur, &p, &where)) {
      continue;
    }
    else if (tokenize_keyword("for", TK_FOR, &cur, &p, &where)) {
      continue;
    }

    // 変数
    if (is_1st_letter_of_symbol(*p)) {
      char *q = p;
      for (; *p && is_letter_of_symbol(*p); p++)
        ;
      cur = new_token(TK_IDENT, cur, q, p - q, &where);
      continue;
    }

    // 演算子
    char const **op;
    for (op = operators; *op; op++) {
      size_t len = strlen(*op);
      if (len <= strlen(p) && !memcmp(p, *op, len)) {
        cur = new_token(TK_RESERVED, cur, p, len, &where);
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
      cur = new_token(TK_NUM, cur, p, org_p - p, &where);
      cur->val = val;
      continue;
    }
    error_at_by_where(where, "トークナイズできません");
  }
  new_token(TK_EOF, cur, p, 0, &where);
  return head.next;
}
