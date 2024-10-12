#include <stdlib.h>

unsigned char atoc(const char *str) {
  int i = atoi(str);
  if (i < 256)
    return i;
  return 255;
}
