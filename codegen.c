////////////////////////////////////////////////////////////////
// Code Generator

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "k9cc.h"

static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

typedef struct GenInfo {
  char *name;
} GenInfo;

static int sequence();
static void emit(const char *fmt, ...);
static void emit_head(void);
static void load();
static void store();
static void gen_addr(Node *node, GenInfo *_info);
static void gen_args(Node *node, GenInfo *info);
static void gen_expr(Node *node, GenInfo *info);
static void gen_stmt(Node *node, GenInfo *info);


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

static void gen_addr(Node *node, GenInfo *info) {
  if (node->kind == ND_VAR) {
    emit("lea rax, [rbp-%d]", node->var->offset);
    emit("push rax");
  }
  else if (node->kind == ND_DEREF) {
    gen_expr(node->lhs, info);
  }
  else {
    error_tok(node->tok, "lvalueではありません");
  }
}

static void gen_args(Node *node, GenInfo *info) {
  static const int nargreg = sizeof(argreg) / sizeof(argreg[1]);
  int nargs = 0;
  for (Node *arg = node; arg; arg = arg->next) {
    nargs++;
    gen_expr(arg, info);
  }
  if (nargreg < nargs) {
    error_tok(node->tok, "number of argument out of range");
  }
  for (int i = nargs - 1; 0 <= i; i--) {
    emit("pop %s", argreg[i]);
  }
}

static void gen_expr(Node *node, GenInfo *info) {
  int seq;
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
  case ND_DEREF:
    gen_expr(node->lhs, info);
    load();
    return;
  case ND_ADDR:
    gen_addr(node->lhs, info);
    return;
  case ND_NUM:
    emit("push %ld", node->val);
    return;
  case ND_FUNCALL:
    seq = sequence();
    gen_args(node->args, info);
    // rspを16の倍数に揃える
    //   判別
    emit("mov rax, rsp");
    emit("and rax, 15");
    emit("jnz .L.noalign_%s%d", info->name, seq);
    //   揃っているとき
    emit("call %s", node->name);
    emit("jmp .L.end_%s%d", info->name, seq);
    //   揃っていなかったとき
    emit(".L.noalign_%s%d:", info->name, seq);
    emit("sub rsp, 8");
    emit("call %s", node->name);
    emit("add rsp, 8");
    // finish
    emit(".L.end_%s%d:", info->name, seq);
    emit("push rax");
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
    error_tok(node->tok, "invalid expression");
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
  case ND_FOR:
    seq = sequence();
    if (node->init) {
      gen_expr(node->init, info);
    }
    emit(".L.begin_%s%d:", info->name, seq);
    if (node->cond) {
      gen_expr(node->cond, info);
      emit("pop rax");
      emit("cmp rax, 0");
      emit("je .L.end_%s%d", info->name, seq);
      gen_stmt(node->then, info);
    }
    if (node->succ) {
      gen_expr(node->succ, info);
    }
    emit("jmp .L.begin_%s%d", info->name, seq);
    emit(".L.end_%s%d:", info->name, seq);
    break;
  case ND_BLOCK:
    for (Node *cur = node->body; cur; cur = cur->next) {
      gen_stmt(cur, info);
    }
    break;
  default:
    error_tok(node->tok, "invalid statement");
  }
}

static void gen_func(Function *fun, GenInfo *info) {
  emit(".global %s", fun->name);
  emit("%s:", fun->name);
  info->name = fun->name;

  // prologue
  emit("push rbp");
  emit("mov rbp, rsp");
  emit("sub rsp, %u", fun->stack_size);

  // params
  int i = 0;
  for (VarList *vl = fun->params; vl; vl = vl->next) {
    emit("mov [rbp-%d], %s", vl->var->offset, argreg[i++]);
  }

  for (Node *cur = fun->node; cur; cur = cur->next) {
    gen_stmt(cur, info);
  }
  emit(".L.return_%s:", info->name);
  emit("mov rsp, rbp");
  emit("pop rbp");
  emit("ret");

}

void codegen(Function *prog) {
  GenInfo info;

  emit(".intel_syntax noprefix");
  for (Function *fun = prog; fun; fun = fun->next) {
    gen_func(fun, &info);
  }
}
