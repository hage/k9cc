#ifndef K9CC_H
#define K9CC_H
#include <stdlib.h>

////////////////////////////////////////////////////////////////
// Token
typedef enum {
  TK_RESERVED,                  // Keywords or punctuators
  TK_NUM,                       // Numeric literals
  TK_EOF,                       // End-of-file markers
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  long val;                     // kindがTK_NUMだったときその値
  char *loc;                    // Token location
  size_t len;                   // Token length
  int column;                   // ソース中の桁番号
};

////////////////////////////////////////////////////////////////
// parser
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM, // numeric
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *lhs;                    // 左辺
  Node *rhs;                    // 右辺
  int val;                      // ND_NUMのときに使う
};

////////////////////////////////////////////////////////////////
/// k9cc.c
extern char *current_input;

////////////////////////////////////////////////////////////////
// report.c
void error(const char *fmt, ...);
void error_tok(Token *tok, const char *fmt, ...);
void error_at(char *pos, const char *fmt, ...);
void va_report(const char*fmt, va_list ap);
void report(const char *fmt, ...);
#define dbgf(fmt, ...)                                                   \
  do {                                                                  \
    report("%s(%d in %s) " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__);    \
  } while(0)
#define dbg(s) report("%s(%d in %s) %s", __FILE__, __LINE__, __func__, s)
#endif
