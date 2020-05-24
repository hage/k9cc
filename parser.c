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
static Node *new_node(NodeKind kind) {
  Node *node = alloc_node();
  node->kind = kind;
  return node;
}

static Node *new_node_binop(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->expr.lhs = lhs;
  node->expr.rhs = rhs;
  return node;
}

static Node *new_node_num(int val) {
  Node *node = new_node_binop(ND_NUM, NULL, NULL);
  node->val = val;
  return node;
}

static Node *new_node_condition(Node *cond, Node *then, Node *els) {
  Node *node = new_node(els ? ND_IFEL : ND_IF);
  node->ifst.cond = cond;
  node->ifst.then_clause = then;
  node->ifst.else_clause = els;
  return node;
}

static Node *new_node_while(Node *cond, Node *body) {
  Node *node = new_node(ND_WHILE);
  node->whilest.cond = cond;
  node->whilest.body = body;
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
static Node *block();
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

// 関数のパラメータを切り出す
static Node *funcparams(Token *tok) {
  if (consume("(")) {
    Node *node = alloc_node();
    node->kind = ND_FUNCALL;
    node->funcall.funcname = tokstrdup(tok);

    if (consume(")")) {
      return node;
    }
    node->funcall.params = calloc(1, sizeof(Code));
    Code *param = node->funcall.params;
    param->node = expr();
    for (;;) {
      if (consume(",")) {
        Code *next_param = calloc(1, sizeof(Code));
        next_param->node = expr();
        param->next = next_param;
        param = next_param;
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
    return NULL;
  }
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

  node = block();
  if (node) {
    return node;
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
      node->forst.init = NULL;
    }
    else {
      node->forst.init = expr();
      expect(";");
    }

    if (consume_if_matched(";", TK_RESERVED)) {
      node->forst.cond = new_node_num(1); // trueを積む
    }
    else {
      node->forst.cond = expr();
      expect(";");
    }

    if (consume_if_matched(")", TK_RESERVED)) {
      node->forst.advance = NULL;
    }
    else {
      node->forst.advance = expr();
      expect(")");
    }
    node->forst.body = stmt();
    return node;
  }
  else if (consume_kind(TK_RETURN)) {
    node = new_node_binop(ND_RETURN, expr(), NULL);
    expect(";");
    return node;
  }
  else {
    node = expr();
    expect(";");
    return node;
  }
}

static Node *block() {
  if (consume("{")) {
    Node *node = new_node_block();
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
  return NULL;
}

static Node *expr() {
  return assign();
}

static Node *assign() {
  Node *node = equality();
  if (consume("=")) {
    node = new_node_binop(ND_ASSIGN, node, assign());
  }
  return node;
}

static Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("==")) {
      node = new_node_binop(ND_EQU, node, relational());
    }
    else if (consume("!=")) {
      node = new_node_binop(ND_NEQ, node, relational());
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
      node = new_node_binop(ND_GRT, node, add());
    }
    else if (consume(">")) {
      node = new_node_binop(ND_GRT, add(), node);
    }
    else if (consume("<=")) {
      node = new_node_binop(ND_GEQ, node, add());
    }
    else if (consume(">=")) {
      node = new_node_binop(ND_GEQ, add(), node);
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
      node = new_node_binop(ND_ADD, node, mul());
    }
    else if (consume("-")) {
      node = new_node_binop(ND_SUB, node, mul());
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
      node = new_node_binop(ND_MUL, node, unary());
    }
    else if (consume("/")) {
      node = new_node_binop(ND_DIV, node, unary());
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
    return new_node_binop(ND_SUB, new_node_num(0), primary());
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
    Node *node = funcparams(tok);
    if (node) {
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
