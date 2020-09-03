////////////////////////////////////////////////////////////////
// Parser

#include <stdlib.h>
#include <string.h>
#include "k9cc.h"

static Node *new_node(ParseInfo *info, NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tok = info->tok;
  return node;
}

static Node *new_binary(ParseInfo *info, NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(info, kind);
  node->lhs = lhs;
  node->rhs = rhs;
  node->tok = info->tok;
  return node;
}

static Node *new_num(ParseInfo *info, long val) {
  Node *node = new_node(info, ND_NUM);
  node->val = val;
  node->tok = info->tok;
  return node;
}

static void walk_one(Node *node, int depth) {
  report("%*s", depth, "");
  if (!node) {
    report("_\n");
    return;
  }
  else if (node->kind == ND_NUM) {
    report("num: %ld\n", node->val);
    return;
  }
  else if (node->kind == ND_VAR) {
    report("var: %s\n", node->var->name);
    return;
  }
  else if (node->kind == ND_IF) {
    report("if:\n");
    report("cond:\n");
    walk_one(node->cond, depth + 2);
    report("then-clause:\n");
    walk_one(node->then, depth + 2);
    if (node->els) {
      report("else-clause:\n");
      walk_one(node->els, depth + 2);
    }
    return;
  }
  else if (node->kind == ND_WHILE) {
    report("while:\n");
    report("cond:\n");
    walk_one(node->cond, depth + 2);
    report("then:\n");
    walk_one(node->then, depth + 2);
    return;
  }
  else if (node->kind == ND_FOR) {
    report("while:\n");
    report("init:\n");
    walk_one(node->init, depth + 2);
    report("cond:\n");
    walk_one(node->cond, depth + 2);
    report("succ:\n");
    walk_one(node->succ, depth + 2);
    report("then:\n");
    walk_one(node->then, depth + 2);
    return;
  }
  else if (node->kind == ND_FUNCALL) {
    report("funcall: %s\n", node->name);
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
  case ND_ASSIGN:
    op = "=";
    break;
  case ND_RETURN:
    op = "return";
    break;
  case ND_EXPR_STMT:
    op = "expr";
    break;
  default:
    report("unknown kind\n");
    return;
  }
  if (op) {
    report("OP[%s]:\n", op);
  }
  walk_real(node->lhs, depth + 2);
  walk_real(node->rhs, depth + 2);
}
void walk_real(Node *node, int depth) {
  for (Node *cur = node; cur; cur = cur->next) {
    walk_one(cur, depth);
  }
}

static Function *funcdef(ParseInfo *info);
static VarList *params(ParseInfo *info);
static Node *stmt(ParseInfo *info);
static Node *expr_stmt(ParseInfo *info);
static Node *expr(ParseInfo *info);
static Node *assign(ParseInfo *info);
static Node *equality(ParseInfo *info);
static Node *relational(ParseInfo *info);
static Node *add(ParseInfo *info);
static Node *mul(ParseInfo *info);
static Node *unary(ParseInfo *info);
static Node *func_args(ParseInfo *info);
static Node *primary(ParseInfo *info);

static ParseInfo *advance_tok(ParseInfo *info) {
  info->tok = info->tok->next;
  return info;
}
static ParseInfo *skip_tok(ParseInfo *info, const char *key) {
  info->tok = skip(info->tok, key);
  return info;
}
static bool at_eot(ParseInfo *info) {
  return info->tok->kind == TK_EOF;
}
static bool peek(ParseInfo *info, const char *key) {
  return info->tok->kind == TK_RESERVED && equal(info->tok, key);
}
static bool consume(ParseInfo *info, const char *key) {
  if (peek(info, key)) {
    skip_tok(info, key);
    return true;
  }
  return false;
}
static char *expect_ident(ParseInfo *info) {
  char *r = identdup(info->tok);
  advance_tok(info);
  return r;
}

// 変数を探す
static Var *find_var(VarList *vl, const char *ident) {
  for (; vl; vl = vl->next) {
    if (strcmp(vl->var->name, ident) == 0) {
      return vl->var;
    }
  }
  return NULL;
}

// 変数を探す。見つからなかったときはエラー
static Var *detect_var(VarList *vl, const char *ident, ParseInfo *info) {
  Var *v = find_var(vl, ident);
  if (!v) {
    error_tok(info->tok, "unknown variable: %s", ident);
  }
  return v;
}

// 変数作る。すでに変数が存在していたときはエラー
static Var *new_var(VarList *vl, const char *ident, ParseInfo *info) {
  Var *v = find_var(vl, ident);

  if (v) {
    error_tok(info->tok, "duplicate variable definition: %s", ident);
  }

  v = calloc(1, sizeof(Var));
  v->name = ident;
  for (VarList *nvl = vl; ; nvl = nvl->next) {
    if (!nvl->next) {
      nvl->next = calloc(sizeof(VarList), 1);
      nvl->next->var = v;
      break;
    }
  }
  return v;
}

// returns stacksize
static int set_locals(VarList *locals) {
  size_t offset = 0;
  for (VarList *v = locals; v; v = v->next) {
    offset += 8;
    v->var->offset = offset;
  }
  return offset;
}

Function *program(Token *tok) {
  Function top, *fun = &top;
  ParseInfo info;
  info.tok = tok;

  while (!at_eot(&info)) {
    fun->next = funcdef(&info);
    fun = fun->next;
  }
  return top.next;
}

// funcdef = "int" ident "(" params? ")" "{" stmt* "}"
static Function *funcdef(ParseInfo *info) {
  skip_tok(info, "int");

  if (info->tok->kind != TK_IDENT) {
    error_tok(info->tok, "need a function definition");
  }
  Function *func = calloc(sizeof(Function), 1);

  func->name = expect_ident(info);
  skip_tok(info, "(");

  // params
  Var var = {0};
  var.name = "";
  VarList vl = {0};
  vl.var = &var;
  info->locals = &vl;
  func->params = params(info);

  skip_tok(info, ")");
  skip_tok(info, "{");


  Node head = {}, *cur = &head, *next;

  while (!consume(info, "}")) {
    next = stmt(info);
    cur->next = next;
    cur = next;
  }
  func->locals = info->locals->next;
  func->stack_size = set_locals(func->locals);
  func->node = head.next;
  return func;
}

// params  = "int" ident ("," "int" ident)*
static VarList *params(ParseInfo *info) {
  if (peek(info, ")")) {
    return NULL;
  }

  skip_tok(info, "int");

  VarList *top, *cur = top = calloc(1, sizeof(VarList));
  cur->var = new_var(info->locals, expect_ident(info), info);
  while (consume(info, ",")) {

    skip_tok(info, "int");

    cur->next = calloc(1, sizeof(VarList));
    cur = cur->next;
    cur->var = new_var(info->locals, expect_ident(info), info);
  }
  return top;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | "int" ident ";"
//      | expr-stmt
static Node *stmt(ParseInfo *info) {
  Node *node;
  if (consume(info, "return")) {
    node = new_node(info, ND_RETURN);
    node->lhs = expr(info);
    skip_tok(info, ";");
    return node;
  }
  else if (consume(info, "if")) {
    node = new_node(info, ND_IF);
    skip_tok(info, "(");
    node->cond = expr(info);
    skip_tok(info, ")");
    node->then = stmt(info);
    if (consume(info, "else")) {
      node->els = stmt(info);
    }
    return node;
  }
  else if (consume(info, "while")) {
    node = new_node(info, ND_WHILE);
    skip_tok(info, "(");
    node->cond = expr(info);
    skip_tok(info, ")");
    node->then = stmt(info);
    return node;
  }
  else if (consume(info, "for")) {
    node = new_node(info, ND_FOR);
    skip_tok(info, "(");

    if (!equal(info->tok, ";")) {
      node->init = expr(info);
    }
    skip_tok(info, ";");

    if (!equal(info->tok, ";")) {
      node->cond = expr(info);
    }
    skip_tok(info, ";");

    if (!equal(info->tok, ")")) {
      node->succ = expr(info);
    }
    skip_tok(info, ")");
    node->then = stmt(info);
    return node;
  }
  else if (consume(info, "{")) {
    node = new_node(info, ND_BLOCK);
    Node top, *cur = &top;
    while (!consume(info, "}")) {
      cur->next = stmt(info);
      cur = cur->next;
    }
    node->body = top.next;
    return node;
  }
  else if (consume(info, "int")) {
    node = new_node(info, ND_NOP);
    new_var(info->locals, expect_ident(info), info);
    skip_tok(info, ";");
    return node;
  }

  return expr_stmt(info);
}

// expr-stmt = expr ";"
static Node *expr_stmt(ParseInfo *info) {
  Node *node = new_node(info, ND_EXPR_STMT);
  node->lhs = expr(info);
  skip_tok(info, ";");
  return node;
}

// expr = assign
static Node *expr(ParseInfo *info) {
  return assign(info);
}

// assign = equality ("=" assign)?
static Node *assign(ParseInfo *info) {
  Node *lhs = equality(info);
  if (consume(info, "=")) {
    Node *rhs = assign(info);
    Node *node = new_binary(info, ND_ASSIGN, lhs, rhs);
    return node;
  }
  else {
    return lhs;
  }
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(ParseInfo *info) {
  Node *node = relational(info);

  for (;;) {
    if (consume(info, "==")) {
      Node *rhs = relational(info);
      node = new_binary(info, ND_EQ, node, rhs);
      continue;
    }
    if (consume(info, "!=")) {
      Node *rhs = relational(info);
      node = new_binary(info, ND_NE, node, rhs);
      continue;
    }
    return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(ParseInfo *info) {
  Node *node = add(info);
  for (;;) {
    if (consume(info, "<")) {
      Node *rhs = add(info);
      node = new_binary(info, ND_LT, node, rhs);
      continue;
    }
    if (consume(info, "<=")) {
      Node *rhs = add(info);
      node = new_binary(info, ND_LE, node, rhs);
      continue;
    }
    if (consume(info, ">")) {
      Node *rhs = add(info);
      node = new_binary(info, ND_LT, rhs, node);
      continue;
    }
    if (consume(info, ">=")) {
      Node *rhs = add(info);
      node = new_binary(info, ND_LE, rhs, node);
      continue;
    }
    return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(ParseInfo *info) {
  Node *node = mul(info);

  for (;;) {
    if (consume(info, "+")) {
      Node *rhs = mul(info);
      node = new_binary(info, ND_ADD, node, rhs);
      continue;
    }
    if (consume(info, "-")) {
      Node *rhs = mul(info);
      node = new_binary(info, ND_SUB, node, rhs);
      continue;
    }
    return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(ParseInfo *info) {
  Node *node = unary(info);

  for (;;) {
    if (consume(info, "*")) {
      Node *rhs = unary(info);
      node = new_binary(info, ND_MUL, node, rhs);
      continue;
    }
    if (consume(info, "/")) {
      Node *rhs = unary(info);
      node = new_binary(info, ND_DIV, node, rhs);
      continue;
    }
    return node;
  }
}
// unary = ("+" | "-") ? primary
//       | "*" unary
//       | "&" unary

static Node *unary(ParseInfo *info) {
  if (consume(info, "-")) {
    return new_binary(info, ND_SUB, new_num(info, 0), primary(info));
  }
  else if (consume(info, "+")) {
    return primary(info);
  }
  else if (consume(info, "*")) {
    Node *node = new_node(info, ND_DEREF);
    node->lhs = unary(info);
    return node;
  }
  else if (consume(info, "&")) {
    Node *node = new_node(info, ND_ADDR);
    node->lhs = unary(info);
    return node;
  }
  else {
    return primary(info);
  }
}

// func-args = "(" assign ("," assign)* ")"
static Node *func_args(ParseInfo *info) {
  skip_tok(info, "(");
  if (consume(info, ")")) {
    return NULL;
  }
  Node *top = assign(info), *n = top;
  while (consume(info, ",")) {
    n->next = assign(info);
    n = n->next;
  }
  skip_tok(info, ")");
  return top;
}

// primary = ident func-args?
//         | "(" expr ")"
//         | num
static Node *primary(ParseInfo *info) {
  if (info->tok->kind == TK_IDENT) {
    char *name = strndup(info->tok->loc, info->tok->len);
    advance_tok(info);
    if (peek(info, "(")) {
      Node *node = new_node(info, ND_FUNCALL);
      node->name = name;
      node->args = func_args(info);
      return node;
    }
    else {
      Var *var = detect_var(info->locals, name, info);
      Node *node = new_node(info, ND_VAR);
      node->var = var;
      return node;
    }
  }
  else if (consume(info, "(")) {
    Node *node = expr(info);
    info->tok = skip(info->tok, ")");
    return node;
  }
  else {
    Node *node = new_num(info, get_number(info->tok));
    advance_tok(info);
    return node;
  }
}
