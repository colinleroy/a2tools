#include <stdlib.h>
#include <stdio.h>

unsigned int get_buf_size(void) {
#ifdef __CC65__
  unsigned int avail = _heapmaxavail();
  if (avail > 5000) {
    return 4096;
  }
  if (avail > 3000) {
    return 2048;
  }
  if (avail > 1500) {
    return 1024;
  }

  return 512;
#else
  return 32768;
#endif
}
