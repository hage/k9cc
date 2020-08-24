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
  report("OP[%s]:\n", op);
  walk_real(node->lhs, depth + 2);
  walk_real(node->rhs, depth + 2);
}
void walk_real(Node *node, int depth) {
  for (Node *cur = node; cur; cur = cur->next) {
    walk_one(cur, depth);
    report("---\n");
  }
}

static Node *program(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);


// program = stmt*
static Node *program(Token **rest, Token *tok) {
  Node head = {}, *cur = &head, *next;
  while (tok->kind != TK_EOF) {
    next = stmt(rest, tok);
    cur->next = next;
    cur = next;
    tok = *rest;
  }
  return head.next;
}

// stmt = "return" expr ";"
//      | expr-stmt
static Node *stmt(Token **rest, Token *tok) {
  if (equal(tok, "return")) {
    Node *node = new_node(ND_RETURN);
    node->lhs = expr(&tok, tok->next);
    *rest = skip(tok, ";");
    return node;
  }
  return expr_stmt(rest, tok);
}

// expr-stmt = expr ";"
static Node *expr_stmt(Token **rest, Token *tok) {
  Node *node = new_node(ND_EXPR_STMT);
  node->lhs = expr(&tok, tok);
  *rest = skip(tok, ";");
  return node;
}

// expr = equality
static Node *expr(Token **rest, Token *tok) {
  return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tok) {
  Node *node = relational(&tok, tok);

  for (;;) {
    if (equal(tok, "==")) {
      Node *rhs = relational(&tok, tok->next);
      node = new_binary(ND_EQ, node, rhs);
      continue;
    }
    if (equal(tok, "!=")) {
      Node *rhs = relational(&tok, tok->next);
      node = new_binary(ND_NE, node, rhs);
      continue;
    }

    *rest = tok;
    return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tok) {
  Node *node = add(&tok, tok);
  for (;;) {
    if (equal(tok, "<")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary(ND_LT, node, rhs);
      continue;
    }
    if (equal(tok, "<=")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary(ND_LE, node, rhs);
      continue;
    }
    if (equal(tok, ">")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary(ND_LT, rhs, node);
      continue;
    }
    if (equal(tok, ">=")) {
      Node *rhs = add(&tok, tok->next);
      node = new_binary(ND_LE, rhs, node);
      continue;
    }

    *rest = tok;
    return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tok) {
  Node *node = mul(&tok, tok);

  for (;;) {
    if (equal(tok, "+")) {
      Node *rhs = mul(&tok, tok->next);
      node = new_binary(ND_ADD, node, rhs);
      continue;
    }
    if (equal(tok, "-")) {
      Node *rhs = mul(&tok, tok->next);
      node = new_binary(ND_SUB, node, rhs);
      continue;
    }
    *rest = tok;
    return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tok) {
  Node *node = unary(&tok, tok);

  for (;;) {
    if (equal(tok, "*")) {
      Node *rhs = unary(&tok, tok->next);
      node = new_binary(ND_MUL, node, rhs);
      continue;
    }
    if (equal(tok, "/")) {
      Node *rhs = unary(&tok, tok->next);
      node = new_binary(ND_DIV, node, rhs);
      continue;
    }
    *rest = tok;
    return node;
  }
}
// unary   = ("+" | "-") ? primary
static Node *unary(Token **rest, Token *tok) {
  if (equal(tok, "-")) {
    return new_binary(ND_SUB, new_num(0), primary(rest, tok->next));
  }
  else if (equal(tok, "+")) {
    return primary(rest, tok->next);
  }
  else {
    return primary(rest, tok);
  }
}

// primary = num | "(" expr ")"
static Node *primary(Token **rest, Token *tok) {
  if (equal(tok, "(")) {
    Node *node = expr(&tok, tok->next);
    *rest = skip(tok, ")");
    return node;
  }
  Node *node = new_num(get_number(tok));
  *rest = tok->next;
  return node;
}

// parser entry
Node *parse(Token *tok) {
  return program(&tok, tok);
}
