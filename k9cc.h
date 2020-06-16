#ifndef K9CC_H
#define K9CC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

////////////////////////////////////////////////////////////////
// typedef

// Variables
typedef enum {
  VAR_AUTO,                     // 自動変数
  VAR_PARAM,                    // 仮引数
} LVarKind;
typedef struct LVar {
  struct LVar *next;
  char *name;                   // 変数の名前
  LVarKind kind;                 // 変数のタイプ (ローカル変数、仮引数等)
  size_t len;                   // 名前の長さ
  size_t offset;                // RBPからのオフセット
} LVar;


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
  TK_T_INT,                     // int型
} TokenKind;

typedef struct TokWhere {
  char *beg_line;               // 行頭
  int column;                // カラム位置
  int line;
} TokWhere;
typedef struct Token Token;
struct Token {
  TokenKind kind;               // トークンの型
  Token *next;                  // 次の入力トークン
  int val;                      // kindがTK_NUMの時、その数値
  char *str;                    // トークン文字列
  size_t len;                   // トークンの長さ
  TokWhere where;               // ファイルでの存在位置
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
  ND_ADDR,                      // 単項&; Node.lhsを使う
  ND_DEREF,                     // 単項*; Node.lhsを使う
  ND_DECLARE,                   // 変数の宣言
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
    Node *lhs;                  // lhs
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
void error_at_by_token(Token *tok, const char *fmt, ...);
void error_at_by_where(TokWhere where, const char *fmt, ...);
void error_at_current(const char *fmt, ...);

// lexer.c
bool consume(char *op);
Token *consume_kind(TokenKind kind);
Token *consume_ident();
bool consume_if_matched(char *op, TokenKind kind);
bool consume_typespec();
void expect_op(char *op);
int expect_number();
bool at_eof();
Token *tokenize(char *p);
const char *tokstrdup(Token *tok);

// parser.c
Funcdef *program();
size_t lvar_top_offset(LVar *locals);

// codegen.c
void codegen(Funcdef *fdef, FILE *fpout);

// k9cc.c
extern char *source_file;

#endif
