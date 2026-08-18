#include <unistd.h>
#include "platform.h"
#include "../qt-serial.h"
#include "../qt-thumbs.h"
#include "../../decoders/qt-conv.h"

extern int ifd;

#pragma code-name(push, "FUJI")
/* No histogram on QT200 thumbs, too expensive */
void fuji_thumb_histogram(void) {
  uint8 x = 0;
  off_t data_offset;
  data_offset = lseek(ifd, 0, SEEK_END) - 160*60;
  lseek(ifd, data_offset, SEEK_SET);

  do {
    x--;
    histogram[x] = x;
  } while (x);
}

void fuji_load_thumb_data(uint8 line) {
  if (!(line & 1)) {
    uint8 i;

    read(ifd, THUMBNAIL_BUF_START, THUMB_WIDTH*2);
    for (i = 0; i < 160; i+=4) {
      THUMBNAIL_BUF_START[i+2] = THUMBNAIL_BUF_START[i+3] = THUMBNAIL_BUF_START[i+1];
      THUMBNAIL_BUF_START[i+1] = THUMBNAIL_BUF_START[i];
    }
  }
}
#pragma code-name(pop)
