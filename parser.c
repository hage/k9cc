#include <string.h>
#include "k9cc.h"

////////////////////////////////////////////////////////////////
// local variables
typedef struct LVar {
  struct LVar *next;
  char *name;                   // 変数の名前
  size_t len;                   // 名前の長さ
  size_t offset;                // RBPからのオフセット
} LVar;

static LVar *locals;

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->len == tok->len && !memcmp(var->name, tok->str, var->len)) {
      return var;
    }
  }
  return NULL;
}

LVar *new_lvar(Token *tok, size_t offset) {
  LVar *lvar = calloc(1, sizeof(LVar));
  lvar->next = locals;
  lvar->name = tok->str;
  lvar->len = tok->len;
  lvar->offset = lvar_top_offset() + offset;
  locals = lvar;
  return lvar;
}

size_t lvar_top_offset() {
  return locals ? locals->offset : 0;
}

////////////////////////////////////////////////////////////////
// create node
static Node *alloc_node() {
  return (Node *)calloc(1, sizeof(Node));
}
static Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = alloc_node();
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM, NULL, NULL);
  node->val = val;
  return node;
}

static Node *new_node_condition(Node *cond, Node *then, Node *els) {
  Node *node = alloc_node();
  node->cond = cond;
  node->then_clause = then;
  node->else_clause = els;
  node->kind = els ? ND_IFEL : ND_IF;
  return node;
}

static Node *new_node_while(Node *cond, Node *then) {
  Node *node = alloc_node();
  node->cond = cond;
  node->then_clause = then;
  node->kind = ND_WHILE;
  return node;
}

static Node *new_node_block() {
  Node *node = alloc_node();
  node->kind = ND_BLOCK;
  return node;
}

////////////////////////////////////////////////////////////////
// parser
static Node *stmt();
static Node *expr();
static Node *assign();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

static Code *new_code(Node *node) {
  Code *c = calloc(1, sizeof(Code));
  c->node = node;
  return c;
}

Code *program() {
  int i;
  Code code, *pc = &code;

  for (i = 0; !at_eof(); i++) {
    pc->next = new_code(stmt());
    pc = pc->next;
  }
  pc->next = NULL;
  return code.next;
}

static Node *stmt() {
  Node *node;

  if (consume("{")) {
    node = new_node_block();
    Code code, *pc = &code;
    for (;;) {
      if (at_eof()) {
        error("ブロックが閉じていません");
      }
      if (consume("}")) {
        pc->next = NULL;
        node->code = code.next;
        return node;
      }
      pc->next = new_code(stmt());
      pc = pc->next;
    }
  }
  else if (consume_kind(TK_IF)) {
    expect("(");
    Node *cond = expr();
    expect(")");
    Node *then = stmt();

    Node *els = NULL;
    if (consume_kind(TK_ELSE)) {
      els = stmt();
    }
    return new_node_condition(cond, then, els);
  }
  else if (consume_kind(TK_WHILE)) {
    expect("(");
    Node *cond = expr();
    expect(")");
    return new_node_while(cond, stmt());
  }
  else if (consume_kind(TK_FOR)) {
    node = alloc_node();
    node->kind = ND_FOR;

    expect("(");
    if (consume_if_matched(";", TK_RESERVED)) {
      node->for_init = NULL;
    }
    else {
      node->for_init = expr();
      expect(";");
    }

    if (consume_if_matched(";", TK_RESERVED)) {
      node->for_cond = new_node_num(1); // trueを積む
    }
    else {
      node->for_cond = expr();
      expect(";");
    }

    if (consume_if_matched(")", TK_RESERVED)) {
      node->for_advance = NULL;
    }
    else {
      node->for_advance = expr();
      expect(")");
    }
    node->for_stmt = stmt();
    return node;
  }
  else if (consume_kind(TK_RETURN)) {
    node = new_node(ND_RETURN, expr(), NULL);
    expect(";");
    return node;
  }
  else {
    node = expr();
    expect(";");
    return node;
  }
}

static Node *expr() {
  return assign();
}

static Node *assign() {
  Node *node = equality();
  if (consume("=")) {
    node = new_node(ND_ASSIGN, node, assign());
  }
  return node;
}

static Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("==")) {
      node = new_node(ND_EQU, node, relational());
    }
    else if (consume("!=")) {
      node = new_node(ND_NEQ, node, relational());
    }
    else {
      return node;
    }
  }
}

static Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<")) {
      node = new_node(ND_GRT, node, add());
    }
    else if (consume(">")) {
      node = new_node(ND_GRT, add(), node);
    }
    else if (consume("<=")) {
      node = new_node(ND_GEQ, node, add());
    }
    else if (consume(">=")) {
      node = new_node(ND_GEQ, add(), node);
    }
    else {
      return node;
    }
  }
}

static Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+")) {
      node = new_node(ND_ADD, node, mul());
    }
    else if (consume("-")) {
      node = new_node(ND_SUB, node, mul());
    }
    else {
      return node;
    }
  }
}

static Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*")) {
      node = new_node(ND_MUL, node, unary());
    }
    else if (consume("/")) {
      node = new_node(ND_DIV, node, unary());
    }
    else {
      return node;
    }
  }
}

static Node *unary() {
  if (consume("+")) {
    return primary();
  }
  if (consume("-")) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }
  return primary();
}

static Node *primary() {
  // 次のトークンが"("なら"(" expr ")"のはず
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    if (consume("(")) {
      // 関数呼び出し
      Node *node = alloc_node();
      node->kind = ND_FUNCALL;
      node->funcname = tokstrdup(tok);
      node->params = calloc(MAX_NPARAMS + 1, sizeof(Node*));

      if (consume(")")) {
        return node;
      }
      int i = 0;
      node->params[i++] = expr();
      while (i < MAX_NPARAMS) {
        if (consume(",")) {
          node->params[i++] = expr();
        }
        else if (at_eof()) {
          error("関数呼び出しが閉じていません");
        }
        else {
          break;
        }
      }
      expect(")");
      return node;
    }
    else {
      // 変数
      Node *node = calloc(1, sizeof(Node));
      node->kind = ND_LVAR;

      LVar *lvar = find_lvar(tok);
      if (lvar) {
        node->offset = lvar->offset;
      }
      else {
        LVar *lvar = new_lvar(tok, 8);
        node->offset = lvar->offset;
      }
      return node;
    }
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}
