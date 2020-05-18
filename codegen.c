#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "k9cc.h"

static FILE *fpout;

static void cprintf(const char *fmt, ...) {
  size_t len = strlen(fmt);
  if (*fmt != '.' && (0 < len && fmt[len - 1] != ':')) {            // ラベルのとき
    fprintf(fpout, "        ");
  }
  va_list ap;
  va_start(ap, fmt);
  vfprintf(fpout, fmt, ap);
  va_end(ap);
  fprintf(fpout, "\n");
}

static int new_label() {
  static int count = 0;
  return count++;
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

static void stack_to_param(int nparam) {
  assert(0 <= nparam && nparam < MAX_NPARAMS);
  static const char *regs[] = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9"
  };
  for (--nparam; 0 <= nparam; nparam--) {
    cprintf("pop %s", regs[nparam]);
  }
}

static void gen(Node *node) {
  int label;
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
    gen_lval(node->e.expr.lhs);
    gen(node->e.expr.rhs);
    cprintf("pop rdi");
    cprintf("pop rax");
    cprintf("mov [rax], rdi");
    cprintf("push rdi");
    return;
  case ND_RETURN:
    gen(node->e.expr.lhs);
    cprintf("pop rax");
    cprintf("mov rsp, rbp");
    cprintf("pop rbp");
    cprintf("ret");
    return;
  case ND_IF:
    label = new_label();
    gen(node->cond);
    cprintf("pop rax");
    cprintf("cmp rax, 0");
    cprintf("je .Lend%04d", label);
    gen(node->then_clause);
    cprintf(".Lend%04d:", label);
    return;
  case ND_IFEL:
    label = new_label();
    gen(node->cond);
    cprintf("pop rax");
    cprintf("cmp rax, 0");
    cprintf("je .Lelse%04d", label);
    gen(node->then_clause);
    cprintf("jmp .Lend%04d", label);
    cprintf(".Lelse%04d:", label);
    gen(node->else_clause);
    cprintf(".Lend%04d:", label);
    return;
  case ND_WHILE:
    label = new_label();
    cprintf(".Lcontinue%04d:", label);
    gen(node->cond);
    cprintf("pop rax");
    cprintf("cmp rax, 0");
    cprintf("je .Lend%04d", label);
    gen(node->then_clause);
    cprintf("jmp .Lcontinue%04d", label);
    cprintf(".Lend%04d:", label);
    return;
  case ND_FOR:
    label = new_label();
    if (node->for_init) {
      gen(node->for_init);
    }
    cprintf(".Lcontinue%04d:", label);

    gen(node->for_cond);
    cprintf("pop rax");
    cprintf("cmp rax, 0");
    cprintf("je .Lend%04d", label);
    gen(node->for_stmt);
    gen(node->for_advance);
    cprintf("jmp .Lcontinue%04d", label);

    cprintf(".Lend%04d:", label);
    return;
  case ND_BLOCK:
    for (Code *pc = node->code; pc; pc = pc->next) {
      gen(pc->node);
      cprintf("pop rax");
    }
    return;
  case ND_FUNCALL: {
    int nparam;
    label = new_label();

    // パラメータを設定する
    for (nparam = 0; node->params[nparam]; nparam++) {
      gen(node->params[nparam]);
    }
    stack_to_param(nparam);

    // rspを16バイト境界に揃える
    cprintf("mov rax, rsp");
    cprintf("and rax, 31");
    cprintf("jnz .Lnoalign%04d", label);

    cprintf("mov rax, rsp");    // すでに16バイト境界に揃っていたとき
    cprintf("call %s", node->funcname);
    cprintf("jmp .Lend%04d", label);

    cprintf(".Lnoalign%04d:", label); //揃っていなかったとき
    cprintf("sub rsp, 8");
    cprintf("mov rax, 0");
    cprintf("call %s", node->funcname);
    cprintf("add rsp, 8");

    // 出口
    cprintf(".Lend%04d:", label);
    cprintf("push rax");
    return;}
  }
  gen(node->e.expr.lhs);
  gen(node->e.expr.rhs);

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
  cprintf(".intel_syntax noprefix");
  cprintf(".global main");
  cprintf("main:");
}

void codegen(Code *code, FILE *fp) {
  fpout = fp;

  // アセンブリ前半部分を出力
  print_header();

  // プロローグ
  cprintf("push rbp");
  cprintf("mov rbp, rsp");
  cprintf("sub rsp, %d", lvar_top_offset());

  // 抽象構文木を下りながらコード生成
  for (; code; code = code->next) {
    gen(code->node);
  }

  // スタックトップ全体の値が残っているので
  // それをRAXに設定して関数からの返り値とする
  cprintf("pop rax");

  // エピローグ
  cprintf("mov rsp, rbp");
  cprintf("pop rbp");
  cprintf("ret");
}
