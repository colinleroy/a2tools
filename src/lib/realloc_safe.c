#include <stdlib.h>
#include <string.h>
#include "malloc0.h"

void *__fastcall__ realloc_safe(void *ptr, size_t size) {
  void *p = realloc(ptr, size);
#ifdef __CC65__
  if (!p && size) {
    __asm__("brk");
  }
#endif
  return p;
}
