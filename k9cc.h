#ifndef K9CC_H
#define K9CC_H

#include <stdlib.h>
#include <stdbool.h>

////////////////////////////////////////////////////////////////
// typedef

// Token
typedef enum {
  TK_RESERVED,                  // 記号
  TK_IDENT,                     // 識別子
  TK_NUM,                       // 整数トークン
  TK_EOF,                       // 入力の終わりを表すトークン
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
} NodeKind;

// 抽象構文木のノードの型
typedef struct Node Node;
struct Node {
  NodeKind kind;                // ノードの型
  Node *lhs;                    // 左辺
  Node *rhs;                    // 右辺
  int val;                      // kindがND_NUMの場合のみ使う
  size_t offset;                // kindがND_LVARの場合のみ使う
};


////////////////////////////////////////////////////////////////
// external valiables
extern char *user_input;        // 入力プログラム
extern Token *token;            // 現在着目しているトークン
extern Node *code[];            // パースしたコード parser.c


////////////////////////////////////////////////////////////////
// functions

// report.c
void pp(const char *fmt, ...);
void error(const char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// lexer.c
bool consume(char *op);
Token *consume_ident();
void expect(char *op);
int expect_number();
bool at_eof();
Token *tokenize(char *p);

// parser.c
Node **program();

// codegen.c
void codegen(Node **node);

#endif
