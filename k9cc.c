#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの種類
typedef enum {
  TK_RESERVED,                  // 記号
  TK_NUM,                       // 整数トークンhc
  TK_EOF,                       // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;               // トークンの型
  Token *next;                  // 次の入力トークン
  int val;                      // kindがTK_NUMの時、その数値
  char *str;                    // トークン文字列
};

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// エラーを報告する関数
// printfと同じ引数を取る
void error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}


/* トークンが期待している記号のときは
 * トークンを1つ読み進めて真を返す。
 * それ以外の場合は偽を返す */
bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op) {
    return false;
  }
  token = token->next;
  return true;
}

/* 次のトークンが期待している記号のときには、トークンを1つ読みすすめる。
 * それ以外のときはエラーを報告する */
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op) {
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
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}


// 入力文字列pをトークナイズして返す
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }
    // 演算子
    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }
    // 数字
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }
    error_at(p, "トークナイズできません");
  }
  new_token(TK_EOF, cur, p);
  return head.next;
}


void print_header(void) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

void cprintf(const char *fmt, ...) {
  printf("        ");
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
  }

  user_input = argv[1];

  // トークナイズする
  token = tokenize(user_input);

  // アセンブリ前半部分を出力
  print_header();

  // 最初の式は数でなければならないので、それをチェックして
  // 最初のmov命令を主力
  cprintf("mov rax, %d", expect_number());

  // `+ <数>` あるいは `- <数>` というトークンの並びを消費しつつ
  // アセンブリを出力
  while (!at_eof()) {
    if (consume('+')) {
      cprintf("add rax, %d", expect_number());
      continue;
    }

    expect('-');
    cprintf("sub rax, %d", expect_number());
  }
  cprintf("ret");
  return 0;
}
