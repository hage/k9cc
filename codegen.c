#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "k9cc.h"


static void cprintf(const char *fmt, ...) {
  printf("        ");
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
}

static void gen_relation(const char *instraction) {
  cprintf("cmp rax, rdi");
  cprintf("%s al", instraction);
  cprintf("movzb rax, al");

}

static void gen_lval(Node *node) {
  if (node->kind != ND_LVAR) {
    error("代入の左辺値が変数ではありません");
  }
  cprintf("mov rax, rbp");
  cprintf("sub rax, %d", node->offset);
  cprintf("push rax");
}

static void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    cprintf("push %d", node->val);
    return;
  case ND_LVAR:
    gen_lval(node);
    cprintf("pop rax");
    cprintf("mov rax, [rax]");
    cprintf("push rax");
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);
    cprintf("pop rdi");
    cprintf("pop rax");
    cprintf("mov [rax], rdi");
    cprintf("push rdi");
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
  case ND_EQU:
    gen_relation("sete");
    break;
  case ND_NEQ:
    gen_relation("setne");
    break;
  case ND_GRT:
    gen_relation("setl");
    break;
  case ND_GEQ:
    gen_relation("setle");
    break;
  }

  cprintf("push rax");
}

static void print_header(void) {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
}

void codegen(Node **node) {
  // アセンブリ前半部分を出力
  print_header();

  // プロローグ
  cprintf("push rbp");
  cprintf("mov rbp, rsp");
  cprintf("sub rsp, %d", lvar_top_offset());

  // 抽象構文木を下りながらコード生成
  for (int i = 0; node[i]; i++) {
    gen(node[i]);
  }

  // スタックトップ全体の値が残っているので
  // それをRAXに設定して関数からの返り値とする
  cprintf("pop rax");

  // エピローグ
  cprintf("mov rsp, rbp");
  cprintf("pop rbp");
  cprintf("ret");
}
