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

// コード列
typedef struct Code {
  Node *node;
  struct Code *next;
} Code;

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

struct Node {
  NodeKind kind;                // ノードの型

  union {
    NKExpr expr;                // expr
    NKIf ifst;                  // if statement
    NKWhile whilest;            // while statement
    NKFor forst;                // for statement
  };

  // block
  Code *code;

  // function name
  const char *funcname;
  Node **params;

  int val;                      // kindがND_NUMの場合のみ使う
  size_t offset;                // kindがND_LVARの場合のみ使う
};

#define MAX_NPARAMS 6           // パラメータの最大個数


////////////////////////////////////////////////////////////////
// external valiables
extern char *user_input;        // 入力プログラム
extern Token *token;            // 現在着目しているトークン


////////////////////////////////////////////////////////////////
// functions

// report.c
void pp(const char *fmt, ...);
void error(const char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

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
Code *program();
size_t lvar_top_offset();
// codegen.c
void codegen(Code *code, FILE *fpout);

#endif
