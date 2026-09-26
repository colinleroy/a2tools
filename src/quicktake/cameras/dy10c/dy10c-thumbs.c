#include <unistd.h>
#include "platform.h"
#include "../qt-serial.h"
#include "../qt-thumbs.h"
#include "../../decoders/qt-conv.h"

#pragma code-name(push, "DY10C")

extern int ifd;

uint8 thumb_len;
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
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
    } while (i--);
  }
}

static void load_fine_thumb(uint8 line) {
  uint8 i, off, c;

  if (!(line & 1)) {
    read(ifd, buffer+256, 40);
    /* Unpack */
    i = 39;
    off = 159;
    do {
      c   = (buffer+256)[i];
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
    } while (i--);
  }
}

static void load_superfine_thumb(uint8 line) {
  uint8 i, off, a, b, c;

  if (!(line & 1)) {
    read(ifd, buffer+256, 80);
    /* Unpack */
    i = 79;
    off = 159;
    do {
      a = (buffer+256)[i] << 6;
      i--;
      b = (buffer+256)[i] >> 2;

      c = a | b;

      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      THUMBNAIL_BUF_START[off--] = c;
      i--;
    } while (i);
  }
}

void dy10c_load_thumb_data(uint8 line) {
  switch (thumb_len) {
    case  (2048 >> 8): load_normal_thumb(line);    break;
    case  (8192 >> 8): load_fine_thumb(line);      break;
    case (10240 >> 8): load_superfine_thumb(line); break;
  }
}
#pragma code-name(pop)
