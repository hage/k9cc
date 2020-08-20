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
  Node *node = parse(tok);

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
