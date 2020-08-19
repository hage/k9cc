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
  int column;                   // ソース中の桁番号
};

////////////////////////////////////////////////////////////////
/// k9cc.c
extern char *current_input;

////////////////////////////////////////////////////////////////
// report.c
void error(const char *fmt, ...);
void error_tok(Token *tok, const char *fmt, ...);
void error_at(char *pos, const char *fmt, ...);
void report(const char *fmt, ...);
#define dbg(format, ...) do {report("%s:%d ", __FILE__, __LINE__); report(format, __VA_ARGS__)} while(0)

#endif
