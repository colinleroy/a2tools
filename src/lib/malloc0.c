#include <stdlib.h>
#include <string.h>
#include "malloc0.h"

void * __fastcall__ malloc0(size_t size) {
  void *p = malloc(size);

  if (p) {
    memset(p, 0, size);
  }
  return p;
}
