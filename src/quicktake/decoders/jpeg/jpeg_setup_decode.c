#include <string.h>
#include "../qt-conv.h"

#define WH_OFFSET 544

char qt_setup_decode(void) {
  /* fix subtypes */
  cache_start[3] &= 0xF0;

  if (memcmp (cache_start, magic, 4)) {
    cputs("Invalid file.\r\n");
    return -1;
  }

  /* FIXME QT 200 implied, 640x480 (scaled down) implied, that sucks */
  width = DECODE_WIDTH;
  height = DECODE_HEIGHT;
  cur_cache_ptr = cache_start; /* Not cache due to stuffback chars */
  return 0;
}
