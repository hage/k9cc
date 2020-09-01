#ifndef K9CC_H
#define K9CC_H
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

////////////////////////////////////////////////////////////////
// lexer.c
extern char *current_input;

typedef enum {
  TK_RESERVED,                  // Keywords or punctuators
  TK_IDENT,                     // Identifires
  TK_NUM,                       // Numeric literals
  TK_EOF,                       // End-of-file markers
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  long val;                     // kindがTK_NUMだったときその値
  const char *loc;              // Token location
  size_t len;                   // Token length
  int column;                   // ソース中の桁番号
};

char *identdup(Token *tok);
long get_number(Token *tok);
bool equal(Token *tok, const char *op);
Token *skip(Token *tok, const char *op);
void dump_token_one(Token *tok);
void dump_token(Token *tok);
Token *tokenize(char *p);

////////////////////////////////////////////////////////////////
// parser.c

// Kind of node
typedef enum {
  ND_ADD,                       // +
  ND_SUB,                       // -
  ND_MUL,                       // *
  ND_DIV,                       // /
  ND_EQ,                        // ==
  ND_NE,                        // !=
  ND_LT,                        // <
  ND_LE,                        // <=
  ND_ASSIGN,                    // =
  ND_ADDR,                      // &
  ND_DEREF,                     // *
  ND_RETURN,                    // "return"
  ND_IF,                        // "if"
  ND_WHILE,                     // "while"
  ND_FOR,                       // "for"
  ND_BLOCK,                     // "{}"
  ND_EXPR_STMT,                 // Expression statement
  ND_VAR,                       // Variable
  ND_NUM,                       // numeric
  ND_FUNCALL,                   // function call
} NodeKind;

// Local variable
typedef struct Var Var;
struct Var {
  const char *name;
  int offset;
};

typedef struct VarList VarList;
struct VarList{
  VarList *next;
  Var *var;
};

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *next;                   // Next node
  Token *tok;

  Node *lhs;                    // 左辺
  Node *rhs;                    // 右辺
  Var *var;                     // ND_VARのときに使う
  int val;                      // ND_NUMのときに使う

  Node *cond;                   // if, while, for
  Node *then;                   // if, while, for
  Node *els;                    // if
  Node *init;                   // for
  Node *succ;                   // for

  Node *body;                   // block

  char *name;                   // funcall
  Node *args;                   // arguments
};

typedef struct ParseInfo {
  Token *tok;
  VarList *locals;
  int stack_size;
} ParseInfo;

typedef struct Function Function;
struct Function {
  Function *next;
  char *name;
  VarList *params;
  Node *node;
  VarList *locals;
  int stack_size;
};

void walk_real(Node *node, int depth);
#define walk(node) walk_real(node, 0)

Function *program(Token *tok);

////////////////////////////////////////////////////////////////
/// codegen.c
void codegen(Function *prog);

////////////////////////////////////////////////////////////////
// report.c
void error(const char *fmt, ...);
void error_tok(Token *tok, const char *fmt, ...);
void error_at(char *pos, const char *fmt, ...);
void va_report(const char*fmt, va_list ap);
void report(const char *fmt, ...);
#define dbgf(fmt, ...)                                                  \
  do {                                                                  \
    report("%s(%d in %s) " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
  } while(0)
#define dbg(s) report("%s(%d in %s) %s", __FILE__, __LINE__, __func__, s)
#endif

////////////////////////////////////////////////////////////////
// utility.c
char *strndup(const char *s, size_t n);
size_t startswith(const char *s, const char *key);
