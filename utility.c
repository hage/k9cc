////////////////////////////////////////////////////////////////
// Utility

#include <stddef.h>
#include <string.h>
#include "k9cc.h"

char *strndup(const char *s, size_t n) {
  return strncpy(calloc(n + 1, 1), s, n);
}

// sがkeyで始まっているときkeyの長さを返す。
// 始まっていないときは0を返す。
size_t startswith(const char *s, const char *key) {
  size_t len = strlen(key);
  return strncmp(s, key, len) == 0 ? len : 0;
}
