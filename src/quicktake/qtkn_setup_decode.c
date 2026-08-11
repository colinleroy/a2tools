#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "qt-conv.h"

#define WH_OFFSET 544

extern uint8 kodak_cbpp;

char qt_setup_decode(void) {
  uint16 v;
  uint16 data_offset;

  if (!memcmp (cache_start, "qktn", 4)) {
    width = 640/2;
    height = 480/2;
    ((unsigned char *)&v)[1] = cache[WH_OFFSET + 8];
    ((unsigned char *)&v)[0] = cache[WH_OFFSET + 9];
    if (v == 30)
      data_offset = 738;
    else
      data_offset = 736;
  } else if (!memcmp (cache_start, "MM\0*", 4)) {
    width = 768/2;
    height = 512/2;
    data_offset = 19712;
    kodak_cbpp = cache[1063] == 243 ? 2:3;
  } else {
    cputs("Invalid file.\r\n");
    return -1;
  }
  lseek(ifd, data_offset, SEEK_SET);
  read(ifd, cur_cache_ptr = cache, CACHE_SIZE);

  return 0;
}
