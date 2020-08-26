////////////////////////////////////////////////////////////////
// Code Generator

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "k9cc.h"

static void emit(const char *fmt, ...) {
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
static void emit_head(void) {
  emit(".intel_syntax noprefix");
  emit(".globl main");
}

static void gen_expr(Node *node) {
  if (node->kind == ND_NUM) {
    emit("push %ld", node->val);
    return;
  }
  gen_expr(node->lhs);
  gen_expr(node->rhs);

  emit("pop rdi");
  emit("pop rax");

  switch (node->kind) {
  case ND_ADD:
    emit("add rax, rdi");
    break;
  case ND_SUB:
    emit("sub rax, rdi");
    break;
  case ND_MUL:
    emit("imul rax, rdi");
    break;
  case ND_DIV:
    emit("cqo");
    emit("idiv rdi");
    break;
  case ND_EQ:
    emit("cmp rax, rdi");
    emit("sete al");
    emit("movzb rax, al");
    break;
  case ND_NE:
    emit("cmp rax, rdi");
    emit("setne al");
    emit("movzb rax, al");
    break;
  case ND_LT:
    emit("cmp rax, rdi");
    emit("setl al");
    emit("movzb rax, al");
    break;
  case ND_LE:
    emit("cmp rax, rdi");
    emit("setle al");
    emit("movzb rax, al");
    break;
  default:
    walk(node);
    error("invalid expression");
  }
  emit("push rax");
}

static void gen_stmt(Node *node) {
  switch (node->kind) {
  case ND_RETURN:
    gen_expr(node->lhs);
    emit("pop rax");
    emit("jmp .L.return");
    break;
  case ND_EXPR_STMT:
    gen_expr(node->lhs);
    emit("add rsp, 8");
    break;
  default:
    error("invalid statement");
  }
}

void codegen(Node *node) {
  emit_head();
  emit("main:");
  for (Node *cur = node; cur; cur = cur->next) {
    gen_stmt(cur);
  }
  emit(".L.return:");
  emit("ret");
}
