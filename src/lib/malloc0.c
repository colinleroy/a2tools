#include <stdlib.h>
#include <string.h>
#include "malloc0.h"

void * __fastcall__ malloc0(size_t size) {
  void *p = malloc(size);
#ifdef __CC65__
  if (!p) {
    __asm__("brk");
  }
#endif
  memset(p, 0, size);
  return p;
}
