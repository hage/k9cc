#ifndef K9CC_H
#define K9CC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

////////////////////////////////////////////////////////////////
// typedef

// Token
typedef enum {
  TK_RESERVED,                  // 記号
  TK_IDENT,                     // 識別子
  TK_NUM,                       // 整数トークン
  TK_RETURN,                    // return
  TK_IF,                        // if
  TK_ELSE,                      // else
  TK_WHILE,                     // while
  TK_FOR,                       // for
  TK_EOF,                       // 入力の終わりを表すトークン
  TK_NONE,                      // 何も意味しない特殊なトークン(peek系でkindを無視するときに使う)
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;               // トークンの型
  Token *next;                  // 次の入力トークン
  int val;                      // kindがTK_NUMの時、その数値
  char *str;                    // トークン文字列
  size_t len;                   // トークンの長さ
};

// Variables
typedef struct LVar {
  struct LVar *next;
  char *name;                   // 変数の名前
  size_t len;                   // 名前の長さ
  size_t offset;                // RBPからのオフセット
} LVar;


// node
typedef enum {
  ND_ADD,                       // +
  ND_SUB,                       // -
  ND_MUL,                       // *
  ND_DIV,                       // /
  ND_EQU,                       // ==
  ND_NEQ,                       // !=
  ND_GRT,                       // >
  ND_GEQ,                       // =>
  ND_ASSIGN,                    // =
  ND_LVAR,                      // ローカル変数
  ND_NUM,                       // 整数
  ND_RETURN,                    // return
  ND_IF,                        // if
  ND_IFEL,                      // elseのあるif
  ND_WHILE,                     // while
  ND_FOR,                       // for
  ND_BLOCK,                     // {}
  ND_FUNCALL,                   // 関数呼び出し
} NodeKind;


// 抽象構文木のノードの型

typedef struct Node Node;
typedef struct Code Code;
typedef struct Funcdef Funcdef;

// コード列
struct Code {
  Node *node;
  Code *next;
};

struct Funcdef {
  Funcdef *next;
  const char *name;
  Code *code;
  LVar *locals;
};


// NodeKind Expr
typedef struct NKExpr {
  Node *lhs;                    // 左辺
  Node *rhs;                    // 右辺
} NKExpr;

// NodeKind If
typedef struct NKIf {
  Node *cond;
  Node *then_clause;
  Node *else_clause;
} NKIf;

// NodeKind While
typedef struct NKWhile {
  Node *cond;
  Node *body;
} NKWhile;

// NodeKind For
typedef struct NKFor {
  Node *init;
  Node *cond;
  Node *advance;
  Node *body;
} NKFor;

// NodeKind Funcall
typedef struct NKFuncall {
  const char *funcname;
  Node *args;                  // パラメータのリスト
} NKFuncall;

struct Node {
  NodeKind kind;                // ノードの型
  Node *next;

  union {
    NKExpr expr;                // expr
    NKIf ifst;                  // if statement
    NKWhile whilest;            // while statement
    NKFor forst;                // for statement
    NKFuncall funcall;          // function call
    Code *code;                 // block
    int val;                    // kindがND_NUMの場合のみ使う
    size_t offset;              // kindがND_LVARの場合のみ使う
  };
};

#define MAX_NPARAMS 6           // パラメータの最大個数


////////////////////////////////////////////////////////////////
// external valiables
extern char *user_input;        // 入力プログラム
extern Token *token;            // 現在着目しているトークン


////////////////////////////////////////////////////////////////
// functions

// report.c
void warn(const char *fmt, ...);
void pp(const char *fmt, ...);
void error(const char *fmt, ...);
void error_at(const char *loc, const char *fmt, ...);
void error_at_by_token(Token *tok, const char *fmt, ...);

// lexer.c
bool consume(char *op);
Token *consume_kind(TokenKind kind);
Token *consume_ident();
bool consume_if_matched(char *op, TokenKind kind);
void expect(char *op);
int expect_number();
bool at_eof();
Token *tokenize(char *p);
const char *tokstrdup(Token *tok);

// parser.c
Funcdef *program();
size_t lvar_top_offset(LVar *locals);
// codegen.c
void codegen(Funcdef *fdef, FILE *fpout);

#endif
