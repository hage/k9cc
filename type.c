#include "k9cc.h"

Type *new_int_type() {
  static Type int_type = {INT, NULL};
  return &int_type;
}
Type *pointer_to(Type *base) {
  Type *ty = calloc(1, sizeof(Type));
  ty->ptr_to = base;
  return ty;
}
