#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 入力プログラム
char *user_input;

void cprintf(const char *fmt, ...) {
  printf("        ");
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

// エラーを報告する関数
// printfと同じ引数を取る
void pp(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

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


////////////////////////////////////////////////////////////////
// Token

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
  size_t len;                   // トークンの長さ
};

// 現在着目しているトークン
Token *token;

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
Token *new_token(TokenKind kind, Token *cur, char *str, size_t len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}


// 入力文字列pをトークナイズして返す
Token *tokenize(char *p) {
  static const char *operators[] = {
    "==", "!=", "<=", ">=",
    "<", ">",
    "-", "+", "/", "*", "(", ")",
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


////////////////////////////////////////////////////////////////
// node

typedef enum {
  ND_ADD,                       // +
  ND_SUB,                       // -
  ND_MUL,                       // *
  ND_DIV,                       // /
  ND_NUM,                       // 整数
} NodeKind;

// 抽象構文木のノードの型
typedef struct Node Node;
struct Node {
  NodeKind kind;                // ノードの型
  Node *lhs;                    // 左辺
  Node *rhs;                    // 右辺
  int val;                      // kindがND_NUMの場合のみ使う
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM, NULL, NULL);
  node->val = val;
  return node;
}

Node *expr();
Node *mul();
Node *unary();
Node *primary();

Node *expr() {
  Node *node = mul();

  for (;;) {
    if (consume("+")) {
      node = new_node(ND_ADD, node, mul());
    }
    else if (consume("-")) {
      node = new_node(ND_SUB, node, mul());
    }
    else {
      return node;
    }
  }
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*")) {
      node = new_node(ND_MUL, node, unary());
    }
    else if (consume("/")) {
      node = new_node(ND_DIV, node, unary());
    }
    else {
      return node;
    }
  }
}

Node *unary() {
  if (consume("+")) {
    return primary();
  }
  if (consume("-")) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }
  return primary();
}

Node *primary() {
  // 次のトークンが"("なら"(" expr ")"のはず
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }
  // そうでなければ数値のはず
  return new_node_num(expect_number());
}


void gen(Node *node) {
  if (node->kind == ND_NUM) {
    cprintf("push %d", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  cprintf("pop rdi");
  cprintf("pop rax");

  switch (node->kind) {
  case ND_ADD:
    cprintf("add rax, rdi");
    break;
  case ND_SUB:
    cprintf("sub rax, rdi");
    break;
  case ND_MUL:
    cprintf("imul rax, rdi");
    break;
  case ND_DIV:
    cprintf("cqo");
    cprintf("idiv rdi");
    break;
  case ND_NUM:
    // not reach
    break;
  }

  cprintf("push rax");
}

////////////////////////////////////////////////////////////////

void print_header(void) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
  }

  // トークナイズしてパースする
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  // アセンブリ前半部分を出力
  print_header();

  // 抽象構文木を下りながらコード生成
  gen(node);

  // スタックトップ錦全体の値が残っているので
  // それをRAXに設定して関数からの返り値とする
  cprintf("pop rax");
  cprintf("ret");
  return 0;
}
