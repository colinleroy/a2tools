#include <unistd.h>
#include "platform.h"
#include "../qt-serial.h"
#include "../qt-thumbs.h"
#include "../../decoders/qt-conv.h"

extern int ifd;

#pragma code-name(push, "DC50")
/* No histogram on DC50 thumbs, too expensive */
void dc50_thumb_histogram(void) {
  uint8 x = 0;
  lseek(ifd, 96*2, SEEK_SET);

  do {
    x--;
    histogram[x] = x;
  } while (x);
}

void dc50_load_thumb_data(uint8 line) {
  if (!(line & 1)) {
    uint8 i, x;

    read(ifd, buffer+256, 96);
    for (i = 8, x = 0; x < 80; ) {
      THUMBNAIL_BUF_START[x] = buffer[i+256] & 0xF0;
      THUMBNAIL_BUF_START[x+1] = buffer[i+256] & 0xF0;
      i++;
      THUMBNAIL_BUF_START[x+2] = buffer[i+256] << 4;
      THUMBNAIL_BUF_START[x+3] = buffer[i+256] << 4;
      i+=2;
      x+=4;
    }
  }
}
#pragma code-name(pop)
