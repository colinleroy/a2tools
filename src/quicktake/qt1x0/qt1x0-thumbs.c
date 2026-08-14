#include <unistd.h>
#include <string.h>
#include "platform.h"
#include "qt-conv.h"
#include "qt-serial.h"
#include "qt-thumbs.h"

extern uint8 is_qt100;
extern int ifd;

#ifndef __CC65__
uint8 *cur_thumb_data;
#endif

/* FIXME this should be in QT1X0 segment */
#pragma code-name(push, "QT1X0")

void thumb_histogram_qt1x0(void) {
  uint8 r_bytes;
  uint16 curr_hist = 0;
  register uint8 x;

#ifndef __CC65__
  bzero(histogram, sizeof(histogram));
#else
  bzero(err_buf, sizeof(err_buf));
#endif

  /* First count values */
  while ((r_bytes = read(ifd, buffer, 255)) != 0) {
#ifndef __CC65__
    cur_thumb_data = buffer;
    do {
      uint8 v = *cur_thumb_data;
      histogram[((v&0x0F) << 4)]++;
      histogram[((v&0xF0))]++;
      cur_thumb_data++;
    } while (r_bytes--);
#else
    __asm__("ldy %v", r_bytes);
    next_byte:
    __asm__("lda %v-%b,y", buffer, 1); /* read byte - offset -1 to go to zero */
    __asm__("sta tmp1"); /* backup it */
    __asm__("asl"); /* << 4 low nibble */
    __asm__("asl");
    __asm__("asl");
    __asm__("asl");

    __asm__("tax");
    __asm__("inc %v,x", err_buf);
    __asm__("bne %g", noof22);
    __asm__("inc %v+256,x", err_buf);
    noof22:

    __asm__("lda tmp1");
    __asm__("and #$F0");
    __asm__("tax");
    __asm__("inc %v,x", err_buf);
    __asm__("bne %g", noof23);
    __asm__("inc %v+256,x", err_buf);
    noof23:

    __asm__("dey");
    __asm__("bne %g", next_byte);
#endif
  }
  /* Rewind for later load */
  lseek(ifd, 0, SEEK_SET);

  x = 0;

  /* Now equalize */
#ifndef __CC65__
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
#else
    // curr_hist += histogram_low[x]|(histogram_high[x]<<8);
    __asm__("lda #$00");
next:
    __asm__("sta %v", x);
    __asm__("tax");
    __asm__("clc");
    __asm__("lda %v,x", err_buf);
    __asm__("adc %v", curr_hist);
    __asm__("sta %v", curr_hist);
    __asm__("lda %v+256,x", err_buf);
    __asm__("adc %v+1", curr_hist);
    __asm__("sta %v+1", curr_hist);
    __A__ = curr_hist / 20;
    // opt_histogram[x] = tmp;
    __asm__("ldx %v", x);
    __asm__("sta %v,x", opt_histogram);
    __asm__("txa");
    __asm__("clc");
    __asm__("adc #$10");
    __asm__("bne %g", next);
#endif
}

/* FIXME this should be in QT1X0 segment */
#pragma code-name(pop)
void load_thumbnail_data_qt1x0(uint8 line) {
#ifndef __CC65__
  uint8 a, b, c, d, dx, i;
#endif
  /* assume thumbnail at 4bpp and zoom it */
  if (is_qt100) {
    if (!(line & 1)) {
      read(ifd, THUMBNAIL_BUF_START, THUMB_WIDTH / 2);
      /* Unpack */
#ifndef __CC65__
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
#else
      __asm__("ldy #39");
      __asm__("ldx #156");

next_thumb_x:
      __asm__("lda %v+%b,y", buffer, THUMBNAIL_BUFFER_OFFSET); /* Load byte at index Y */
      __asm__("sta tmp2");  /* backup value */
      __asm__("asl");       /* low nibble, << 4 */
      __asm__("asl");
      __asm__("asl");
      __asm__("asl");
      __asm__("sta %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("dex");
      __asm__("sta %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET);

      __asm__("lda tmp2");  /* restore value */
      __asm__("and #$F0"); /* high nibble */
      __asm__("dex");
      __asm__("sta %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("dex");
      __asm__("sta %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("dex");

      __asm__("dey");
      __asm__("bpl %g", next_thumb_x);
#endif
    }
  } else { /* QT150 */
#ifndef __CC65__
    unsigned char *cur_in, *cur_out;
#endif
    /* Whyyyyyy do they do that */
    if (!(line % 4)) {
      /* Expand the next two lines from 4bpp thumb_buf to 8bpp buffer */
      read(ifd, thumb_buf, THUMB_WIDTH);
#ifndef __CC65__
      cur_in = thumb_buf;
      cur_out = buffer+THUMBNAIL_BUFFER_OFFSET;
      for (dx = 0; dx < THUMB_WIDTH; dx++) {
        c = *cur_in++;
        a   = (c & 0xF0);
        *cur_out++ = a;
        b   = ((c & 0x0F) << 4);
        *cur_out++ = b;
      }
#else
      __asm__("ldx #0");
      __asm__("ldy #0");
next_expand_x:
      __asm__("lda %v,x", thumb_buf);
      __asm__("sta tmp1");
      __asm__("and #$F0"); /* high nibble */
      __asm__("sta %v+%b,y", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("iny");

      __asm__("lda tmp1");  /* low nibble, << 4 */
      __asm__("asl");
      __asm__("asl");
      __asm__("asl");
      __asm__("asl");
      __asm__("sta %v+%b,y", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("iny");

      __asm__("inx");
      __asm__("cpx #%b", THUMB_WIDTH);
      __asm__("bcc %g", next_expand_x);
#endif

      /* Reorder bytes from buffer back to thumb_buf */
#ifndef __CC65__
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
#else
      __asm__("lda #>%v", thumb_buf);
      __asm__("sta %g+2", out_high_page);

      __asm__("ldx #0");  /* in */
      __asm__("ldy #0");  /* out */
next_three_quarters_x:
      __asm__("lda %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("sta %v,y", thumb_buf);

      __asm__("inx");
      __asm__("iny");
      __asm__("lda %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("sta %v,y", thumb_buf);

      __asm__("inx");
      __asm__("lda %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("sta %v+%b,y", thumb_buf, THUMB_WIDTH-1);

      __asm__("iny");
      __asm__("inx");

      __asm__("cpx #%b", THUMB_WIDTH*3/2);
      __asm__("bcc %g", next_three_quarters_x);

last_quarter_x:
      __asm__("iny");
      __asm__("lda %v+%b,x", buffer, THUMBNAIL_BUFFER_OFFSET);
out_high_page:
      __asm__("sta %v,y", thumb_buf);
      __asm__("iny");
      __asm__("inx");
      __asm__("bne %g", check_bound);
      __asm__("inc %g+2", out_high_page);
check_bound:
      __asm__("cpx #<%w", (THUMB_WIDTH*2));
      __asm__("bcc %g", last_quarter_x);

#endif

      /* Finally copy the first line of thumb_buf to buffer for display,
       * upscaling horizontally */
#ifndef __CC65__
      cur_in = thumb_buf;
      cur_out = THUMBNAIL_BUF_START;
      for (dx = 0; dx < THUMB_WIDTH; dx++) {
        *cur_out = *cur_in;
        cur_out++;
        *cur_out = *cur_in;
        cur_out++;
        cur_in++;
      }
#else
      __asm__("ldx #0");
      __asm__("ldy #0");
next_copy_x:
      __asm__("lda %v,x", thumb_buf);
      __asm__("sta %v+%b,y", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("iny");
      __asm__("sta %v+%b,y", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("iny");
      __asm__("inx");
      __asm__("cpx #%b", THUMB_WIDTH);
      __asm__("bcc %g", next_copy_x);
#endif

    } else if (!(line % 2)) {
      /* Copy the second line of thumb_buf to buffer for display,
       * upscaling horizontally */
#ifndef __CC65__
      cur_in = thumb_buf + THUMB_WIDTH;
      cur_out = THUMBNAIL_BUF_START;
      for (dx = 0; dx < THUMB_WIDTH; dx++) {
        *cur_out = *cur_in;
        cur_out++;
        *cur_out = *cur_in;
        cur_out++;
        cur_in++;
      }
#else
      __asm__("ldx #0");
      __asm__("ldy #0");
next_copy_x2:
      __asm__("lda %v+%b,x", thumb_buf, THUMB_WIDTH);
      __asm__("sta %v+%b,y", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("iny");
      __asm__("sta %v+%b,y", buffer, THUMBNAIL_BUFFER_OFFSET);
      __asm__("iny");
      __asm__("inx");
      __asm__("cpx #%b", THUMB_WIDTH);
      __asm__("bcc %g", next_copy_x2);
#endif
    } else {
      /* Reuse the previous buffer line once for upscaling */
    }
  }
}
