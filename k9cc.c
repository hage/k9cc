#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "k9cc.h"

////////////////////////////////////////////////////////////////
// Utility
char *strndup(const char *s, size_t n) {
  return strncpy(calloc(n + 1, 1), s, n);
}

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
    static char *operators[] = {
      "+", "-", "*", "/",
      "(", ")",
      "==", "!=",
      "<=", ">=",
      ">", "<",
      NULL
    };
    char **op;
    for (op = operators; *op; op++) {
      size_t len = strlen(*op);
      if (!strncmp(p, *op, len)) {
        cur = new_token(TK_RESERVED, cur, p, len, column);
        p += len;
        break;
      }
    }
    if (!*op) {
      error_at(p, "invalid punctures");
    }
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
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM, // numeric
} NodeKind;

// AST node type
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
Node *equality(Token **rest, Token *tok);
Node *relational(Token **rest, Token *tok);
Node *add(Token **rest, Token *tok);
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
  case ND_EQ:
    op = "==";
    break;
  case ND_NE:
    op = "!=";
    break;
  case ND_LT:
    op = "<";
    break;
  case ND_LE:
    op = "<=";
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

// expr = equality
Node *expr(Token **rest, Token *tok) {
  return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality(Token **rest, Token *tok) {
  Node *node = relational(&tok, tok);

  for (;;) {
    if (equal(tok, "==")) {
      Node *rhs = relational(&tok, tok->next);
      node = new_binary(ND_EQ, node, rhs);
      continue;
    }
    if (equal(tok, "!=")) {
      Node *rhs = relational(&tok, tok->next);
      node = new_binary(ND_NE, node, rhs);
      continue;
    }

    *rest = tok;
    return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational(Token **rest, Token *tok) {
  Node *node = add(&tok, tok);
  for (;;) {
    if (equal(tok, "<")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary(ND_LT, node, rhs);
      continue;
    }
    if (equal(tok, "<=")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary(ND_LE, node, rhs);
      continue;
    }
    if (equal(tok, ">")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary(ND_LT, rhs, node);
      continue;
    }
    if (equal(tok, ">=")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary(ND_LE, rhs, node);
      continue;
    }

    *rest = tok;
    return node;
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add(Token **rest, Token *tok) {
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

// mul = unary ("*" unary | "/" unary)*
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
    return new_binary(ND_SUB, new_num(0), primary(rest, tok->next));
  }
  else if (equal(tok, "+")) {
    return primary(rest, tok->next);
  }
  else {
    return primary(rest, tok);
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
  case ND_SUB:
    emit("sub %s, %s", rd, rs);
    return;
  case ND_MUL:
    emit("imul %s, %s", rd, rs);
    return;
  case ND_DIV:
    emit("mov rax, %s", rd);
    emit("cqo");
    emit("idiv %s", rs);
    emit("mov %s, rax", rd);
    return;
  case ND_EQ:
    emit("cmp %s, %s", rd, rs);
    emit("sete al");
    emit("movzb %s, al", rd);
    return;
  case ND_NE:
    emit("cmp %s, %s", rd, rs);
    emit("setne al");
    emit("movzb %s, al", rd);
    return;
  case ND_LT:
    emit("cmp %s, %s", rd, rs);
    emit("setl al");
    emit("movzb %s, al", rd);
    return;
  case ND_LE:
    emit("cmp %s, %s", rd, rs);
    emit("setle al");
    emit("movzb %s, al", rd);
    return;
  default:
    walk(node);
    error("invalid expression");
  }
}

////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません\n");
  }
  Token *tok = tokenize(argv[1]), *toktop = tok;
  Node *node = expr(&tok, tok);

  // dump_token(toktop);
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
