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
    /* Whyyyyyy do they do that */
    if (!(line % 4)) {
      /* Expand the next two lines from 4bpp thumb_buf to 8bpp buffer */
      read(ifd, thumb_buf, THUMB_WIDTH);
      cur_in = thumb_buf;
      cur_out = buffer+THUMBNAIL_BUFFER_OFFSET;
      for (dx = 0; dx < THUMB_WIDTH; dx++) {
        c = *cur_in++;
        a   = (c & 0xF0);
        *cur_out++ = a;
        b   = ((c & 0x0F) << 4);
        *cur_out++ = b;
      }

      /* Reorder bytes from buffer back to thumb_buf */
      cur_in = buffer+THUMBNAIL_BUFFER_OFFSET;
      cur_out = thumb_buf;
      for (i = 0; i < THUMB_WIDTH*3/2; ) {
        a = *cur_in++;
        i++;

        *(cur_out) = a;
        cur_out++;

        b = *cur_in++;
        *(cur_out) = b;
        i++;

        c = *cur_in++;
        *(cur_out + THUMB_WIDTH-1) = c;
        i++;

        cur_out++;
      }

      for (; i < THUMB_WIDTH * 2; ) {
        cur_out++;
        d = *cur_in++;
        *(cur_out) = d;
        cur_out++;

        i++;
      }

qt150_second_line:
      /* Finally copy the first line of thumb_buf to buffer for display,
       * upscaling horizontally */
      cur_in = thumb_buf;
      cur_out = THUMBNAIL_BUF_START;
      for (dx = 0; dx < THUMB_WIDTH; dx++) {
        *cur_out = *cur_in;
        cur_out++;
        *cur_out = *cur_in;
        cur_out++;
        cur_in++;
      }
    } else if (!(line % 2)) {
      goto qt150_second_line;
    } else {
      /* Reuse the previous buffer line once for upscaling */
    }
  }
}
