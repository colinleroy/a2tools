#include <unistd.h>
#include "platform.h"
#include "qt-conv.h"
#include "qt-serial.h"
#include "qt-thumbs.h"

extern int ifd;

#pragma code-name(push, "QT200")

/* No histogram on QT200 thumbs, too expensive */
void thumb_histogram_qt200(void) {
  uint8 x = 0;
#ifndef __CC65__
  do {
    x--;
    histogram[x] = x;
  } while (x);
#else
  __asm__("ldx #0");
next:
  __asm__("txa");
  __asm__("sta %v,x", opt_histogram);
  __asm__("inx");
  __asm__("bne %g", next);
#endif
}

void load_thumbnail_data_qt200(uint8 line) {
  if (!(line & 1)) {
    read(ifd, THUMBNAIL_BUF_START, THUMB_WIDTH*2);
#ifndef __CC65__
    uint8 i;
    for (i = 0; i < 160; i+=4) {
      THUMBNAIL_BUF_START[i+2] = THUMBNAIL_BUF_START[i+3] = THUMBNAIL_BUF_START[i+1];
      THUMBNAIL_BUF_START[i+1] = THUMBNAIL_BUF_START[i];
    }
#else
    __asm__("ldy #39");
next:
    __asm__("tya");
    __asm__("asl");
    __asm__("asl");
    __asm__("tax");
    __asm__("lda %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET+1);
    __asm__("sta %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET+2);
    __asm__("sta %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET+3);
    __asm__("lda %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET);
    __asm__("sta %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET+1);
    __asm__("dey");
    __asm__("bpl %g", next);
#endif
  }
}
#pragma code-name(pop)
