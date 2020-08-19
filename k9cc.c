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

Token *skip(Token *tok, const char *op) {
  if (!equal(tok, op)) {
    error_tok(tok, "expected '%s'", op);
  }
  return tok->next;
}

void dump_token_real(Token *tok, const char *file, int line) {
  report("%s(%d): ", file, line);
  switch (tok->kind) {
  case TK_RESERVED:
    report("[TK_RESERVED] %s\n", tok->loc);
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
#define dump_token(tok) dump_token_real(tok, __FILE__, __LINE__)

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
    if (strchr("+-*/()", *p)) {
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
// parser
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // numeric
} NodeKind;

typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *lhs;                    // 左辺
  Node *rhs;                    // 右辺
  int val;                      // ND_NUMのときに使う
};

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_num(long val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

Node *expr(Token **rest, Token *tok);
Node *mul(Token **rest, Token *tok);
Node *unary(Token **rest, Token *tok);
Node *primary(Token **rest, Token *tok);

void walk_real(Node *node, int depth) {
  report("%*s", depth, "");
  if (node->kind == ND_NUM) {
    report("num: %ld\n", node->val);
    return;
  }
  char *op;
  switch (node->kind) {
  case ND_ADD:
    op = "+";
    break;
  case ND_SUB:
    op = "-";
    break;
  case ND_MUL:
    op = "*";
    break;
  case ND_DIV:
    op = "/";
    break;
  default:
    report("unknown kind\n");
    return;
  }
  report("OP[%s]:\n", op);
  walk_real(node->lhs, depth + 2);
  walk_real(node->rhs, depth + 2);
}
#define walk(node) walk_real(node, 0)

// expr = mul ("+" mul | "-" mul)*
Node *expr(Token **rest, Token *tok) {
  Node *node = mul(&tok, tok);

  for (;;) {
    if (equal(tok, "+")) {
      Node *rhs = mul(&tok, tok->next);
      node = new_binary(ND_ADD, node, rhs);
      continue;
    }
    if (equal(tok, "-")) {
      Node *rhs = mul(&tok, tok->next);
      node = new_binary(ND_SUB, node, rhs);
      continue;
    }
    *rest = tok;
    return node;
  }
}

// mul     = unary ("*" unary | "/" unary)*
Node *mul(Token **rest, Token *tok) {
  Node *node = unary(&tok, tok);

  for (;;) {
    if (equal(tok, "*")) {
      Node *rhs = unary(&tok, tok->next);
      node = new_binary(ND_MUL, node, rhs);
      continue;
    }
    if (equal(tok, "/")) {
      Node *rhs = unary(&tok, tok->next);
      node = new_binary(ND_DIV, node, rhs);
      continue;
    }
    *rest = tok;
    return node;
  }
}
// unary   = ("+" | "-") ? primary
Node *unary(Token **rest, Token *tok) {
  if (equal(tok, "-")) {
    Node *lhs = new_num(0);
    Node *rhs = primary(rest, tok->next);
    return new_binary(ND_SUB, lhs, rhs);
  }
  else if (equal(tok, "+")) {
    Node *node = primary(rest, tok->next);
    return node;
  }
  else {
    Node *node = primary(rest, tok);
    return node;
  }
}

// primary = num | "(" expr ")"
Node *primary(Token **rest, Token *tok) {
  if (equal(tok, "(")) {
    Node *node = expr(&tok, tok->next);
    *rest = skip(tok, ")");
    return node;
  }
  Node *node = new_num(get_number(tok));
  *rest = tok->next;
  return node;
}

////////////////////////////////////////////////////////////////
// Code Generator
static char *reg(int idx) {
  static char *r[] = {"r10", "r11", "r12", "r13", "r14", "r15"};
  if (idx < 0 || sizeof(r) / sizeof(*r) <= (unsigned)idx) {
    error("register out of range: %d", idx);
  }
  return r[idx];
}
static int top;
void gen_expr(Node *node) {
  if (node->kind == ND_NUM) {
    emit("mov %s, %d", reg(top++), node->val);
    return;
  }
  gen_expr(node->lhs);
  gen_expr(node->rhs);
  char *rd = reg(top - 2);
  char *rs = reg(top - 1);
  top--;
  switch (node->kind) {
  case ND_ADD:
    emit("add %s, %s", rd, rs);
    return;
    break;
  case ND_SUB:
    emit("sub %s, %s", rd, rs);
    return;
    break;
  case ND_MUL:
    emit("imul %s, %s", rd, rs);
    return;
    break;
  case ND_DIV:
    emit("mov rax, %s", rd);
    emit("cqo");
    emit("idiv %s", rs);
    emit("mov %s, rax", rd);
    return;
    break;
  default:
    error("invalid expression");
  }
}

////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません\n");
  }
  Token *tok = tokenize(argv[1]);
  Node *node = expr(&tok, tok);

  // walk(node);

  emit_head();
  emit("main:");
  emit("push r12");
  emit("push r13");
  emit("push r14");
  emit("push r15");

  gen_expr(node);
  emit("mov rax, %s", reg(top - 1));

  emit("pop r15");
  emit("pop r14");
  emit("pop r13");
  emit("pop r12");
  emit("ret");
  return 0;
}

// Local Variables:
// compile-command: "./dr make test"
// End:
