#include <string.h>
#include "qt-conv.h"

#define WH_OFFSET 544

char qt_setup_decode(void) {
  uint16 v;

  if (memcmp (cache_start, magic, 4)) {
    cputs("Invalid file.\r\n");
    return -1;
  }

  /* QTKN pictures are always 640x480 */
  width = DECODE_WIDTH;
  height = DECODE_HEIGHT;

  ((unsigned char *)&v)[1] = cache[WH_OFFSET + 8];
  ((unsigned char *)&v)[0] = cache[WH_OFFSET + 9];

  if (v == 30)
    cur_cache_ptr = cache + (738);
  else
    cur_cache_ptr = cache + (736);

  return 0;
}
