////////////////////////////////////////////////////////////////
// Code Generator

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "k9cc.h"

typedef struct GenInfo {
  char *name;
} GenInfo;

static int sequence() {
  static int seq = 0;
  return seq++;
}

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


static void load() {
  emit("pop rax");
  emit("mov rax, [rax]");
  emit("push rax");
}

static void store() {
  emit("pop rdi");
  emit("pop rax");
  emit("mov [rax], rdi");
  emit("push rdi");
}

static void gen_addr(Node *node, GenInfo *_info) {
  if (node->kind == ND_VAR) {
    emit("lea rax, [rbp-%d]", node->var->offset);
    emit("push rax");
  }
  else {
    error("lvalueではありません");
  }
}

static void gen_expr(Node *node, GenInfo *info) {
  switch (node->kind) {
  case ND_ASSIGN:
    gen_addr(node->lhs, info);
    gen_expr(node->rhs, info);
    store();
    return;
  case ND_VAR:
    gen_addr(node, info);
    load();
    return;
  case ND_NUM:
    emit("push %ld", node->val);
    return;
  }

  gen_expr(node->lhs, info);
  gen_expr(node->rhs, info);

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

static void gen_stmt(Node *node, GenInfo *info) {
  int seq;
  switch (node->kind) {
  case ND_RETURN:
    gen_expr(node->lhs, info);
    emit("pop rax");
    emit("jmp .L.return_%s", info->name);
    break;
  case ND_EXPR_STMT:
    gen_expr(node->lhs, info);
    emit("add rsp, 8");
    break;
  case ND_IF:
    gen_expr(node->cond, info);
    emit("pop rax");
    emit("cmp rax, 0");

    if (node->els) {
      seq = sequence();
      emit("je .L.else_%s%d", info->name, seq);
      gen_stmt(node->then, info);
      emit("jmp .L.end_%s%d", info->name, seq);
      emit(".L.else_%s%d:", info->name, seq);
      gen_stmt(node->els, info);
      emit(".L.end_%s%d:", info->name, seq);
    }
    else {
      seq = sequence();
      emit("je .L.end_%s%d", info->name, seq);
      gen_stmt(node->then, info);
      emit(".L.end_%s%d:", info->name, seq);
    }
  case ND_WHILE:
    seq = sequence();
    emit(".L.while_%s%d:", info->name, seq);
    gen_expr(node->cond, info);
    emit("pop rax");
    emit("cmp rax, 0");
    emit("je .L.end_%s%d", info->name, seq);
    gen_stmt(node->then, info);
    emit("jmp .L.while_%s%d", info->name, seq);
    emit(".L.end_%s%d:", info->name, seq);
    break;
  default:
    error("invalid statement");
  }
}

void codegen(Function *prog) {
  GenInfo info;
  info.name = "main";

  emit_head();
  emit("main:");

  // prologue
  emit("push rbp");
  emit("mov rbp, rsp");
  emit("sub rsp, %u", prog->stack_size);
  for (Node *cur = prog->node; cur; cur = cur->next) {
    gen_stmt(cur, &info);
  }
  emit(".L.return_%s:", info.name);
  emit("mov rsp, rbp");
  emit("pop rbp");
  emit("ret");
}
