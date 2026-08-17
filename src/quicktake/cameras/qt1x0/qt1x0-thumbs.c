#include <unistd.h>
#include <string.h>
#include "platform.h"
#include "../../decoders/qt-conv.h"
#include "../qt-serial.h"
#include "../qt-thumbs.h"

extern uint8 is_qt100;
extern int ifd;

uint8 *cur_thumb_data;

#pragma code-name(push, "QT1X0")
void qt1x0_thumb_histogram(void) {
  register signed char page;
  register unsigned char x;
  uint16 curr_hist = 0;
  uint16 read_len;

  if (!is_qt100) {
    /* On QT150, thumbnails are 40x30 16bpp (4R/8G/4B). We only keep
     * the green channel, and those 8bits are hard to reassemble, so
     * we skip histograming. */
    x = 0;
    do {
      opt_histogram[x] = x;
      x--;
    } while(x);

    return;
  }
  bzero(histogram, sizeof(histogram));

  /* First count values */
  uint8 r_bytes;
  while ((r_bytes = read(ifd, buffer, 255)) != 0) {
    cur_thumb_data = buffer;
    do {
      uint8 v = *cur_thumb_data;
      histogram[((v&0x0F) << 4)]++;
      histogram[((v&0xF0))]++;
      cur_thumb_data++;
    } while (r_bytes--);
  }

  /* Rewind for later load */
  lseek(ifd, 0, SEEK_SET);

  x = 0;
  /* Now equalize */
  do {
    uint32 tmp_large;
    uint16 tmp;
    curr_hist += histogram[x];
    // tmp_large = ((uint32)curr_hist * 0xF0);
    // tmp = tmp_large >> 6; /* /64 */
    // tmp /= 75;            /* /64/75 = /80/60 */
    // opt_histogram[x] = tmp;
    opt_histogram[x] = curr_hist/20;
    x += 0x10;
  } while (x);
}

/* FIXME this should be in QT1X0 segment */
#pragma code-name(pop)
void qt1x0_load_thumb_data(uint8 line) {
  uint8 a, b, c, d, dx, i;
  /* assume thumbnail at 4bpp and zoom it */
  if (is_qt100) {
    if (!(line & 1)) {
      read(ifd, THUMBNAIL_BUF_START, THUMB_WIDTH / 2);
      /* Unpack */
      uint8 off;
      i = 39;
      do {
        c   = THUMBNAIL_BUF_START[i];
        a   = (c & 0xF0);
        b   = (c << 4);
        off = i * 4;
        THUMBNAIL_BUF_START[off++] = a;
        THUMBNAIL_BUF_START[off++] = a;
        THUMBNAIL_BUF_START[off++] = b;
        THUMBNAIL_BUF_START[off] = b;
      } while (i--);
    }
  } else { /* QT150 */
    unsigned char *cur_in, *cur_out;
    /* Note this is not interpolating. See .s file or
     * loader.c for an interpolating implementation */
    if (!(line % 4)) {
      unsigned char x;
      /* Expand the next two lines from RGGB thumb_buf to 8bpp buffer */
      read(ifd, thumb_buf, THUMB_WIDTH);

      /* Thumbnails are 40*30, 16bit color, with RGGB (4/8/4 bits). they're encoded
       * in sequence of 40*RGG followed by 40*B. We want to keep G to grayscale at
       * 8bpp.
       * This means we need to discard 4 bits/keep 8 bits 40 times, then discard 20 bytes.
       *
       * RRRRGGGG GGGGRRRR GGGGGGGG RRRRGGGG GGGGRRRR GGGGGGGG RRRRGGGG GGGGRRRR...
       */
      for (i = 0, x = 0; i < 60;) {
        unsigned char g;

        g = (thumb_buf[i] & 0x0F) | (thumb_buf[i+1] & 0xF0);
        THUMBNAIL_BUF_START[x]   = g;
        THUMBNAIL_BUF_START[x+1] = g;
        THUMBNAIL_BUF_START[x+2] = g;
        THUMBNAIL_BUF_START[x+3] = g;

        g = thumb_buf[i+2];
        THUMBNAIL_BUF_START[x+4] = g;
        THUMBNAIL_BUF_START[x+5] = g;
        THUMBNAIL_BUF_START[x+6] = g;
        THUMBNAIL_BUF_START[x+7] = g;

        i += 3;
        x += 8;
      }

    }
  }
}
