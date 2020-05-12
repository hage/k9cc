#include <stdio.h>
int foo(int a, int b, int c) {
  int val = a + b * c;
  printf("%d", val);
  return val;
}
