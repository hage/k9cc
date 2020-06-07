#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "k9cc.h"

static FILE *fpout;

static const char *param_regs[] = {
  "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};

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
  if (MAX_NPARAMS < nparam) {
    error("パラメータが%d個以上あります", MAX_NPARAMS + 1);
  }
  for (--nparam; 0 <= nparam; nparam--) {
    cprintf("pop %s", param_regs[nparam]);
  }
}

static void gen(Node *node) {
  int label;
  switch (node->kind) {
  case ND_NUM:
    cprintf("push %d", node->val);
    return;
  case ND_ADDR:
    gen_lval(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    cprintf("pop rax");
    cprintf("mov rax, [rax]");
    cprintf("push rax");
    return;
  case ND_LVAR:
    gen_lval(node);
    cprintf("pop rax");
    cprintf("mov rax, [rax]");
    cprintf("push rax");
    return;
  case ND_ASSIGN:
    gen_lval(node->expr.lhs);
    gen(node->expr.rhs);
    cprintf("pop rdi");
    cprintf("pop rax");
    cprintf("mov [rax], rdi");
    cprintf("push rdi");
    return;
  case ND_RETURN:
    gen(node->expr.lhs);
    cprintf("pop rax");
    cprintf("mov rsp, rbp");
    cprintf("pop rbp");
    cprintf("ret");
    return;
  case ND_IF:
    label = new_label();
    gen(node->ifst.cond);
    cprintf("pop rax");
    cprintf("cmp rax, 0");
    cprintf("je .Lend%04d", label);
    gen(node->ifst.then_clause);
    cprintf(".Lend%04d:", label);
    return;
  case ND_IFEL:
    label = new_label();
    gen(node->ifst.cond);
    cprintf("pop rax");
    cprintf("cmp rax, 0");
    cprintf("je .Lelse%04d", label);
    gen(node->ifst.then_clause);
    cprintf("jmp .Lend%04d", label);
    cprintf(".Lelse%04d:", label);
    gen(node->ifst.else_clause);
    cprintf(".Lend%04d:", label);
    return;
  case ND_WHILE:
    label = new_label();
    cprintf(".Lcontinue%04d:", label);
    gen(node->whilest.cond);
    cprintf("pop rax");
    cprintf("cmp rax, 0");
    cprintf("je .Lend%04d", label);
    gen(node->whilest.body);
    cprintf("jmp .Lcontinue%04d", label);
    cprintf(".Lend%04d:", label);
    return;
  case ND_FOR:
    label = new_label();
    if (node->forst.init) {
      gen(node->forst.init);
    }
    cprintf(".Lcontinue%04d:", label);

    gen(node->forst.cond);
    cprintf("pop rax");
    cprintf("cmp rax, 0");
    cprintf("je .Lend%04d", label);
    gen(node->forst.body);
    gen(node->forst.advance);
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
    int nparam = 0;
    label = new_label();

    // パラメータを設定する
    for (Node *arg = node->funcall.args; arg; arg = arg->next) {
      gen(arg);
      nparam++;
    }
    stack_to_param(nparam);

    // rspを16バイト境界に揃える
    cprintf("mov rax, rsp");
    cprintf("and rax, 31");
    cprintf("jnz .Lnoalign%04d", label);

    cprintf("mov rax, rsp");    // すでに16バイト境界に揃っていたとき
    cprintf("call %s", node->funcall.funcname);
    cprintf("jmp .Lend%04d", label);

    cprintf(".Lnoalign%04d:", label); //揃っていなかったとき
    cprintf("sub rsp, 8");
    cprintf("mov rax, 0");
    cprintf("call %s", node->funcall.funcname);
    cprintf("add rsp, 8");

    // 出口
    cprintf(".Lend%04d:", label);
    cprintf("push rax");
    return;}
  }
  gen(node->expr.lhs);
  gen(node->expr.rhs);

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

static void funcgen(Funcdef *fdef) {
  // アセンブリ前半部分を出力
  cprintf(".global %s", fdef->name);
  cprintf("%s:", fdef->name);

  // プロローグ
  cprintf("push rbp");
  cprintf("mov rbp, rsp");
  cprintf("sub rsp, %d", lvar_top_offset(fdef->locals));

  // 仮引数をスタックに積む
  int nparam = 0;
  LVar *lvar;
  for (lvar = fdef->locals; lvar; lvar = lvar->next) {
    if (lvar->type == VAR_PARAM) {
      nparam++;
    }
  }
  if (MAX_NPARAMS < nparam) {
    error("パラメータが%d個以上あります", MAX_NPARAMS + 1);
  }
  for (lvar = fdef->locals; lvar; lvar = lvar->next) {
    if (lvar->type == VAR_PARAM) {
      nparam--;
      const char *reg = param_regs[nparam];
      cprintf("mov rax, rbp");
      cprintf("sub rax, %d", lvar->offset);
      cprintf("mov [rax], %s", reg);
    }
  }


  // 抽象構文木を下りながらコード生成
  for (Code *code = fdef->code; code; code = code->next) {
    gen(code->node);
  }

  // エピローグ
  cprintf("mov rsp, rbp");
  cprintf("pop rbp");
  cprintf("ret");
}

void codegen(Funcdef *fdef, FILE *fp) {
  fpout = fp;
  cprintf(".intel_syntax noprefix");
  for (; fdef; fdef = fdef->next) {
    // pp("%s", fdef->name);
    funcgen(fdef);
  }
}
