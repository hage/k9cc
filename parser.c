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
  if (node->kind == ND_NUM) {
    report("num: %ld\n", node->val);
    return;
  }
  else if (node->kind == ND_VAR) {
    report("var: %c\n", node->name);
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

static Node *program(ParseInfo *info);
static Node *stmt(ParseInfo *info);
static Node *expr_stmt(ParseInfo *info);
static Node *expr(ParseInfo *info);
static Node *assign(ParseInfo *info);
static Node *equality(ParseInfo *info);
static Node *relational(ParseInfo *info);
static Node *add(ParseInfo *info);
static Node *mul(ParseInfo *info);
static Node *unary(ParseInfo *info);
static Node *primary(ParseInfo *info);

static ParseInfo *advance_tok(ParseInfo *info) {
  info->tok = info->tok->next;
  return info;
}

// program = stmt*
static Node *program(ParseInfo *info) {
  Node head = {}, *cur = &head, *next;
  while (info->tok->kind != TK_EOF) {
    next = stmt(info);
    cur->next = next;
    cur = next;
  }
  return head.next;
}

// stmt = "return" expr ";"
//      | expr-stmt
static Node *stmt(ParseInfo *info) {
  if (equal(info->tok, "return")) {
    Node *node = new_node(ND_RETURN);
    node->lhs = expr(advance_tok(info));
    info->tok = skip(info->tok, ";");
    return node;
  }
  return expr_stmt(info);
}

// expr-stmt = expr ";"
static Node *expr_stmt(ParseInfo *info) {
  Node *node = new_node(ND_EXPR_STMT);
  node->lhs = expr(info);
  info->tok = skip(info->tok, ";");
  return node;
}

// expr = assign
static Node *expr(ParseInfo *info) {
  return assign(info);
}

// assign = equality ("=" assign)?
static Node *assign(ParseInfo *info) {
  Node *lhs = equality(info);
  if (equal(info->tok, "=")) {
    Node *rhs = assign(advance_tok(info));
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
    if (equal(info->tok, "==")) {
      Node *rhs = relational(advance_tok(info));
      node = new_binary(ND_EQ, node, rhs);
      continue;
    }
    if (equal(info->tok, "!=")) {
      Node *rhs = relational(advance_tok(info));
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
    if (equal(info->tok, "<")) {
      Node *rhs = add(advance_tok(info));
      node = new_binary(ND_LT, node, rhs);
      continue;
    }
    if (equal(info->tok, "<=")) {
      Node *rhs = add(advance_tok(info));
      node = new_binary(ND_LE, node, rhs);
      continue;
    }
    if (equal(info->tok, ">")) {
      Node *rhs = add(advance_tok(info));
      node = new_binary(ND_LT, rhs, node);
      continue;
    }
    if (equal(info->tok, ">=")) {
      Node *rhs = add(advance_tok(info));
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
    if (equal(info->tok, "+")) {
      Node *rhs = mul(advance_tok(info));
      node = new_binary(ND_ADD, node, rhs);
      continue;
    }
    if (equal(info->tok, "-")) {
      Node *rhs = mul(advance_tok(info));
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
    if (equal(info->tok, "*")) {
      Node *rhs = unary(advance_tok(info));
      node = new_binary(ND_MUL, node, rhs);
      continue;
    }
    if (equal(info->tok, "/")) {
      Node *rhs = unary(advance_tok(info));
      node = new_binary(ND_DIV, node, rhs);
      continue;
    }
    return node;
  }
}
// unary   = ("+" | "-") ? primary
static Node *unary(ParseInfo *info) {
  if (equal(info->tok, "-")) {
    return new_binary(ND_SUB, new_num(0), primary(advance_tok(info)));
  }
  else if (equal(info->tok, "+")) {
    return primary(advance_tok(info));
  }
  else {
    return primary(info);
  }
}

// primary = ident | num | "(" expr ")"
static Node *primary(ParseInfo *info) {
  if (equal(info->tok, "(")) {
    Node *node = expr(advance_tok(info));
    info->tok = skip(info->tok, ")");
    return node;
  }
  else if (info->tok->kind == TK_IDENT) {
    Node *node = new_node(ND_VAR);
    node->name = *info->tok->loc;
    advance_tok(info);
    return node;
  }
  Node *node = new_num(get_number(info->tok));
  advance_tok(info);
  return node;
}

// parser entry
Node *parse(Token *tok) {
  ParseInfo info;
  info.tok = tok;
  return program(&info);
}
