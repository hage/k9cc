#include <string.h>
#include "k9cc.h"

////////////////////////////////////////////////////////////////
// local variables
static LVar *find_lvar(LVar *locals, Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->len == tok->len && !memcmp(var->name, tok->str, var->len)) {
      return var;
    }
  }
  return NULL;
}

static LVar *new_lvar(LVar **plocals, VarType type, Token *tok, size_t offset) {
  LVar *lvar = calloc(1, sizeof(LVar));
  lvar->next = *plocals;
  lvar->name = tok->str;
  lvar->type = type;
  lvar->len = tok->len;
  lvar->offset = lvar_top_offset(*plocals) + offset;
  *plocals = lvar;
  return lvar;
}

size_t lvar_top_offset(LVar *locals) {
  return locals ? locals->offset : 0;
}

////////////////////////////////////////////////////////////////
// create node
static Node *new_node(NodeKind kind) {
  Node *node = (Node *)calloc(1, sizeof(Node));
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
  Node *node = new_node(ND_BLOCK);
  return node;
}

////////////////////////////////////////////////////////////////
// parser
static Funcdef *funcdef();
static Node *stmt(LVar **plocals);
static Node *block(LVar **plocals);
static Node *expr(LVar **plocals);
static Node *assign(LVar **plocals);
static Node *equality(LVar **plocals);
static Node *relational(LVar **plocals);
static Node *add(LVar **plocals);
static Node *mul(LVar **plocals);
static Node *unary(LVar **plocals);
static Node *primary(LVar **plocals);

static Code *new_code(Node *node) {
  Code *c = calloc(1, sizeof(Code));
  c->node = node;
  return c;
}

// 関数の実引数を切り出す
static Node *funcargs(Token *tok, LVar **plocals) {
  if (consume("(")) {
    Node *node = new_node(ND_FUNCALL);
    node->funcall.funcname = tokstrdup(tok);

    if (consume(")")) {
      return node;
    }
    Node args, *cur = &args;
    cur = cur->next = assign(plocals);
    for (;;) {
      if (consume(",")) {
        cur = cur->next = assign(plocals);
      }
      else if (at_eof()) {
        error("関数呼び出しが閉じていません");
      }
      else {
        cur->next = NULL;
        break;
      }
    }
    expect(")");
    node->funcall.args = args.next;
    return node;
  }
  else {
    return NULL;
  }
}


Funcdef *program() {
  Funcdef fun, *cur = &fun;
  while (!at_eof()) {
    cur = cur->next = funcdef();
  }
  return fun.next;
}

static Funcdef *funcdef() {
  Code code, *cur = &code;
  Funcdef *fun = calloc(1, sizeof(Funcdef));
  Token *tok = consume_ident();
  LVar *locals = NULL;

  if (tok) {
    fun->name = tokstrdup(tok);

    // params
    expect("(");
    Token *tok_param = consume_ident();
    while (tok_param) {
      new_lvar(&locals, VAR_PARAM, tok_param, 8);
      if (consume(",")) {
        tok_param = consume_ident();
      }
      else if (at_eof()) {
        error_at_by_token(tok, "関数 %s の仮引数が閉じていません", fun->name);
      }
      else {
	break;
      }
    }
    expect(")");

    // body
    expect("{");
    for (;;) {
      if (at_eof()) {
        error_at_by_token(tok, "関数 %s が閉じていません", fun->name);
      }
      else if (consume("}")) {
        break;
      }
      cur = cur->next = new_code(stmt(&locals));
    }
  }
  fun->code = code.next;
  fun->locals = locals;
  return fun;
}

static Node *stmt(LVar **plocals) {
  Node *node;

  if (consume_if_matched("int", TK_IDENT)) {
    Node *node = new_node(ND_DECLARE);
    Token *tok = consume_ident();
    if (tok) {
      expect(";");
      new_lvar(plocals, VAR_AUTO, tok, 8);
    }
    else {
      error_at_by_token(token, "変数宣言には変数名が必要です");
    }
    return node;
  }

  node = block(plocals);
  if (node) {
    return node;
  }
  else if (consume_kind(TK_IF)) {
    expect("(");
    Node *cond = expr(plocals);
    expect(")");
    Node *then = stmt(plocals);

    Node *els = NULL;
    if (consume_kind(TK_ELSE)) {
      els = stmt(plocals);
    }
    return new_node_condition(cond, then, els);
  }
  else if (consume_kind(TK_WHILE)) {
    expect("(");
    Node *cond = expr(plocals);
    expect(")");
    return new_node_while(cond, stmt(plocals));
  }
  else if (consume_kind(TK_FOR)) {
    node = new_node(ND_FOR);

    expect("(");
    if (consume_if_matched(";", TK_RESERVED)) {
      node->forst.init = NULL;
    }
    else {
      node->forst.init = expr(plocals);
      expect(";");
    }

    if (consume_if_matched(";", TK_RESERVED)) {
      node->forst.cond = new_node_num(1); // trueを積む
    }
    else {
      node->forst.cond = expr(plocals);
      expect(";");
    }

    if (consume_if_matched(")", TK_RESERVED)) {
      node->forst.advance = NULL;
    }
    else {
      node->forst.advance = expr(plocals);
      expect(")");
    }
    node->forst.body = stmt(plocals);
    return node;
  }
  else if (consume_kind(TK_RETURN)) {
    node = new_node_binop(ND_RETURN, expr(plocals), NULL);
    expect(";");
    return node;
  }
  else {
    node = expr(plocals);
    expect(";");
    return node;
  }
}

static Node *block(LVar **plocals) {
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
      pc->next = new_code(stmt(plocals));
      pc = pc->next;
    }
  }
  return NULL;
}

static Node *expr(LVar **plocals) {
  return assign(plocals);
}

static Node *assign(LVar **plocals) {
  Node *node = equality(plocals);
  if (consume("=")) {
    node = new_node_binop(ND_ASSIGN, node, assign(plocals));
  }
  return node;
}

static Node *equality(LVar **plocals) {
  Node *node = relational(plocals);

  for (;;) {
    if (consume("==")) {
      node = new_node_binop(ND_EQU, node, relational(plocals));
    }
    else if (consume("!=")) {
      node = new_node_binop(ND_NEQ, node, relational(plocals));
    }
    else {
      return node;
    }
  }
}

static Node *relational(LVar **plocals) {
  Node *node = add(plocals);

  for (;;) {
    if (consume("<")) {
      node = new_node_binop(ND_GRT, node, add(plocals));
    }
    else if (consume(">")) {
      node = new_node_binop(ND_GRT, add(plocals), node);
    }
    else if (consume("<=")) {
      node = new_node_binop(ND_GEQ, node, add(plocals));
    }
    else if (consume(">=")) {
      node = new_node_binop(ND_GEQ, add(plocals), node);
    }
    else {
      return node;
    }
  }
}

static Node *add(LVar **plocals) {
  Node *node = mul(plocals);

  for (;;) {
    if (consume("+")) {
      node = new_node_binop(ND_ADD, node, mul(plocals));
    }
    else if (consume("-")) {
      node = new_node_binop(ND_SUB, node, mul(plocals));
    }
    else {
      return node;
    }
  }
}

static Node *mul(LVar **plocals) {
  Node *node = unary(plocals);

  for (;;) {
    if (consume("*")) {
      node = new_node_binop(ND_MUL, node, unary(plocals));
    }
    else if (consume("/")) {
      node = new_node_binop(ND_DIV, node, unary(plocals));
    }
    else {
      return node;
    }
  }
}

static Node *unary(LVar **plocals) {
  if (consume("+")) {
    return primary(plocals);
  }
  if (consume("-")) {
    return new_node_binop(ND_SUB, new_node_num(0), primary(plocals));
  }
  if (consume("*")) {
    Node *node = new_node(ND_DEREF);
    Node *u = unary(plocals);
    node->lhs = u;
    return node;
  }
  if (consume("&")) {
    Node *node = new_node(ND_ADDR);
    Node *u = unary(plocals);
    node->lhs = u;
    return node;
  }
  return primary(plocals);
}

static Node *primary(LVar **plocals) {
  // 次のトークンが"("なら"(" expr ")"のはず
  if (consume("(")) {
    Node *node = expr(plocals);
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    Node *node = funcargs(tok, plocals);
    if (node) {
      return node;
    }
    else {
      // 変数
      Node *node = new_node(ND_LVAR);

      LVar *lvar = find_lvar(*plocals, tok);
      if (lvar) {
        node->offset = lvar->offset;
      }
      else {
        error_at_by_token(tok, "%s: 宣言していない変数です", tokstrdup(tok));
      }
      return node;
    }
  }

  // そうでなければ数値のはず
  return new_node_num(expect_number());
}
