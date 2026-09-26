#include <unistd.h>
#include "platform.h"
#include "../qt-serial.h"
#include "../qt-thumbs.h"
#include "../../decoders/qt-conv.h"

#pragma code-name(push, "DY10C")

extern int ifd;

uint8 thumb_len;
#ifndef __CC65__
void dy10c_thumb_histogram(void) {
  uint8 x = 0;
  thumb_len = lseek(ifd, 0, SEEK_END) >> 8;
  lseek(ifd, 0, SEEK_SET);

  do {
    x--;
    histogram[x] = x;
  } while (x);
}

static void load_normal_thumb(uint8 line) {
  uint8 i, off, c;
  if (!(line & 3)) {
    read(ifd, buffer+256, 20);
    /* Unpack */
    i = 19;
    off = 159;
    do {
      c   = (buffer+256)[i];
      THUMBNAIL_BUF_START[off--] =
        THUMBNAIL_BUF_START[off--] =
        THUMBNAIL_BUF_START[off--] =
        THUMBNAIL_BUF_START[off--] =
        THUMBNAIL_BUF_START[off--] =
        THUMBNAIL_BUF_START[off--] =
        THUMBNAIL_BUF_START[off--] =
        THUMBNAIL_BUF_START[off--] = c;
    } while (i--);
  }
}
#else
void load_normal_thumb(uint8 line);
#endif

static void load_fine_thumb(uint8 line) {
  uint8 i, x, c;
  read(ifd, buffer+256, 40);
  for (i = 0, x = 0; x < 80;) {
    c   = (buffer+256)[i];
    THUMBNAIL_BUF_START[x] =
      THUMBNAIL_BUF_START[x+1] = c;

    x+=2;
    i++;
  }
}

static void load_superfine_thumb(uint8 line) {
}

void dy10c_load_thumb_data(uint8 line) {
  switch (thumb_len) {
    case  (2048 >> 8): load_normal_thumb(line);    break;
    case  (8192 >> 8): load_fine_thumb(line);      break;
    case (10240 >> 8): load_superfine_thumb(line); break;
  }
}
#pragma code-name(pop)
