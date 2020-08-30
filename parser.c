////////////////////////////////////////////////////////////////
// Parser

#include <stdlib.h>
#include <string.h>
#include "k9cc.h"

static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_num(long val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
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

static Var *find_var(Var *var, const char *ident) {
  for (; var; var = var->next) {
    if (strcmp(var->name, ident) == 0) {
      return var;
    }
  }
  return NULL;
}

static Var *find_or_new_var(Var *var, const char *ident) {
  Var *v = find_var(var, ident);
  if (!v) {
    v = calloc(sizeof(Var), 1);
    v->name = ident;
    for (Var *w = var; ; w = w->next) {
      if (!w->next) {
        w->next = v;
        break;
      }
    }
  }
  return v;
}

// program = stmt*
Function *program(Token *tok) {
  Var var = {0};
  var.name = "";

  ParseInfo info;
  info.tok = tok;
  info.locals = &var;

  Node head = {}, *cur = &head, *next;

  while (!at_eot(&info)) {
    next = stmt(&info);
    cur->next = next;
    cur = next;
  }
  Function *func = calloc(sizeof(Function), 1);
  func->locals = info.locals->next;
  func->stack_size = 0;
  func->node = head.next;
  return func;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | expr-stmt
static Node *stmt(ParseInfo *info) {
  if (consume(info, "return")) {
    Node *node = new_node(ND_RETURN);
    node->lhs = expr(info);
    skip_tok(info, ";");
    return node;
  }
  else if (consume(info, "if")) {
    Node *node = new_node(ND_IF);
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
    Node *node = new_node(ND_WHILE);
    skip_tok(info, "(");
    node->cond = expr(info);
    skip_tok(info, ")");
    node->then = stmt(info);
    return node;
  }
  else if (consume(info, "for")) {
    Node *node = new_node(ND_FOR);
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
    Node *node = new_node(ND_BLOCK);
    Node top, *cur = &top;
    while (!consume(info, "}")) {
      cur->next = stmt(info);
      cur = cur->next;
    }
    node->body = top.next;
    return node;
  }
  return expr_stmt(info);
}

// expr-stmt = expr ";"
static Node *expr_stmt(ParseInfo *info) {
  Node *node = new_node(ND_EXPR_STMT);
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
    Node *node = new_binary(ND_ASSIGN, lhs, rhs);
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
      node = new_binary(ND_EQ, node, rhs);
      continue;
    }
    if (consume(info, "!=")) {
      Node *rhs = relational(info);
      node = new_binary(ND_NE, node, rhs);
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
      node = new_binary(ND_LT, node, rhs);
      continue;
    }
    if (consume(info, "<=")) {
      Node *rhs = add(info);
      node = new_binary(ND_LE, node, rhs);
      continue;
    }
    if (consume(info, ">")) {
      Node *rhs = add(info);
      node = new_binary(ND_LT, rhs, node);
      continue;
    }
    if (consume(info, ">=")) {
      Node *rhs = add(info);
      node = new_binary(ND_LE, rhs, node);
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
      node = new_binary(ND_ADD, node, rhs);
      continue;
    }
    if (consume(info, "-")) {
      Node *rhs = mul(info);
      node = new_binary(ND_SUB, node, rhs);
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
      node = new_binary(ND_MUL, node, rhs);
      continue;
    }
    if (consume(info, "/")) {
      Node *rhs = unary(info);
      node = new_binary(ND_DIV, node, rhs);
      continue;
    }
    return node;
  }
}
// unary   = ("+" | "-") ? primary
static Node *unary(ParseInfo *info) {
  if (consume(info, "-")) {
    return new_binary(ND_SUB, new_num(0), primary(info));
  }
  else if (consume(info, "+")) {
    return primary(info);
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
      Node *node = new_node(ND_FUNCALL);
      node->name = name;
      node->args = func_args(info);
      return node;
    }
    else {
      Var *var = find_or_new_var(info->locals, name);
      Node *node = new_node(ND_VAR);
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
    Node *node = new_num(get_number(info->tok));
    advance_tok(info);
    return node;
  }
}
