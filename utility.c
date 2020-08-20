////////////////////////////////////////////////////////////////
// Utility

#include <stddef.h>
#include <string.h>
#include "k9cc.h"

char *strndup(const char *s, size_t n) {
  return strncpy(calloc(n + 1, 1), s, n);
}

// Local Variables:
// compile-command: "./dr make test"
// End:
