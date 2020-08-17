#ifndef K9CC_H
#define K9CC_H

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
  int len;                      // Token length
};


////////////////////////////////////////////////////////////////
// report
void error(const char *fmt, ...);

#endif
