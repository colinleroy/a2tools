#include <unistd.h>
#include "platform.h"
#include "../qt-serial.h"
#include "../qt-thumbs.h"
#include "../../decoders/qt-conv.h"

extern int ifd;

#pragma code-name(push, "DC2X0")
/* No histogram on dc2x0 thumbs, too expensive */
void dc2x0_thumb_histogram(void) {
  uint8 x = 0;
  lseek(ifd, 96*6, SEEK_SET);

  do {
    x--;
    histogram[x] = x;
  } while (x);
}

void dc2x0_load_thumb_data(uint8 line) {
  if (!(line & 1)) {
    uint8 i, x;

    read(ifd, buffer+256, 96);
    for (i = 8, x = 0; x < 80; ) {
      THUMBNAIL_BUF_START[x] = buffer[i+256] << 4;
      x++;
      THUMBNAIL_BUF_START[x] = buffer[i+256] << 4;
      i++;
      x++;
    }
  }
}
#pragma code-name(pop)
