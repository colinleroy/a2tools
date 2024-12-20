/*
   QTKT (QuickTake 100) decoding algorithm
   Copyright 2023, Colin Leroy-Mira <colin@colino.net>

   Heavily based on dcraw.c -- Dave Coffin's raw photo decoder
   Copyright 1997-2018 by Dave Coffin, dcoffin a cybercom o net

   Welcome to pointer arithmetic hell - you can refer to dcraw's
   quicktake_100_load_raw() is you prefer array hell
 */

/* Handle pic by horizontal bands for memory constraints reasons.
 * Bands need to be a multiple of 4px high for compression reasons
 * on QT 150/200 pictures,
 * and a multiple of 5px for nearest-neighbor scaling reasons.
 * (480 => 192 = *0.4, 240 => 192 = *0.8)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "progress_bar.h"
#include "qt-conv.h"
#include "qtk_bithuff.h"

/* Shared with qt-conv.c */
char magic[5] = QTKT_MAGIC;
char *model = "100";
uint16 *huff_ptr = NULL; /* unused here, just for linking */
uint8 *dst, *src;
uint8 cache[CACHE_SIZE];
uint8 *cache_start = cache;

/* Pointer arithmetic helpers */
static uint16 width_plus2;
static uint16 pgbar_state;
static uint8 *last_two_lines;
static uint8 at_very_first_row;

/* Decoding stuff. The values from 0-7 are negative
 * but we reverse them when we use them for optimisation
 */
static const int16 gstep[16] =
{ -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };

/* Indexes. We need to follow the current pixelbuf (at idx) at
 * various places:
 * idx_forward is start of the next row,
 * idx_behind is row-1, col-1,
 * idx_behind_plus2 is row-1, col-3,
 * idx_end is where we stop to finish a line
 */
uint8 *idx_forward;
uint8 *idx_behind;
uint8 *idx_end;
uint8 *idx_behind_plus2;
uint16 *idx_pix_rows;


/* Internal data buffer
 * We don't use the destination raw_image directly,
 * because the algorithm uses two pixels bands top,
 * left, right and bottom as scratch. So we need a
 * 24x644 buffer to decode our 20x640 band. We add
 * two pixels on a 25th line because they are re-
 * used at the start of the next band.
 */
#define SCRATCH_WIDTH 645
#define SCRATCH_HEIGHT (BAND_HEIGHT + 4)
uint8 raw_image[SCRATCH_HEIGHT * SCRATCH_WIDTH + 2];

void __fastcall__ reset_bitbuff (void) {
}

void qt_load_raw(uint16 top)
{
  register uint8 *idx;
  register uint8 at_very_first_col;
  register uint8 row;
  register int16 val;
  uint8 ln_val, hn_val;

  /* First band: init variables */
  if (top == 0) {
    reset_bitbuff();

    at_very_first_row = 1;
    width_plus2 = width + 2;
    pgbar_state = 0;

    /* calculate offsets to shift the last two lines + 2px
     * from the end of the previous band to the start of
     * the new one.
     */
    last_two_lines = raw_image + (BAND_HEIGHT * SCRATCH_WIDTH);

    /* Init the second line + 2 bytes of buffer with grey. */
    memset (raw_image+SCRATCH_WIDTH, 0x80, SCRATCH_WIDTH + 2);
  } else {
    /* Shift the last band's last line, plus 2 pixels,
     * to second line of the new band. */
    memcpy(raw_image+SCRATCH_WIDTH, last_two_lines+SCRATCH_WIDTH, SCRATCH_WIDTH + 2);
  }

  /* We start at line 2. */
  src = raw_image + (2 * SCRATCH_WIDTH);

  /* In reality we do rows from 0 to BAND_HEIGHT, but decrementing is faster
   * and the only use of the variable is to check for oddity, so nothing
   * changes */
  for (row = BAND_HEIGHT; row != 0; row--) {
    /* Adapt indexes depending on the row's oddity */
    if (row & 1) {
      idx_forward = src + SCRATCH_WIDTH;
      idx = src + 1;
      idx_end = src + width + 1;
      pgbar_state+=2;
      progress_bar(-1, -1, 80*22, pgbar_state, height);
    } else {
      idx = src;
      idx_end = src + width;
      idx_forward = src + SCRATCH_WIDTH + 1;
    }

    /* Initial set of the value two columns behind */
    ln_val = (*idx);

    /* row-1, col-1 */
    idx_behind = idx - (SCRATCH_WIDTH-1);

    /* Shift source buffer for next line */
    src += SCRATCH_WIDTH;

    at_very_first_col = 1;

    /* First pass */
    while (idx != idx_end) {
      uint8 high_nibble, low_nibble;
      if (cur_cache_ptr == cache_end) {
        read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
      }

      high_nibble = *(cur_cache_ptr++);
      low_nibble = high_nibble & 0x0F;
      high_nibble >>= 4;

      /* Do high nibble */

      val = ((((*idx_behind               // row-1, col-1
              + ln_val) >> 1)
              + *(idx_behind+2)) >> 1) // row-1, col+1
              + gstep[high_nibble];

      if (val < 0)
        val = 0;
      else if (val > 255)
        val = 255;

      /* Cache it for next loop before shifting */
      hn_val = val;

      *(idx+2) = val;

      /* Same for the first line of the image */
      if (at_very_first_row) {
        /* row-1,col+1 / row-1,col+3*/
        *(idx_behind+4) = *(idx_behind+2) = val;
      }

      /* At first columns, we have to set scratch values for the next line.
       * We'll need them in the second pass */
      if (at_very_first_col) {
        *(idx+1) = *(idx_forward) = *(idx) = val;
        at_very_first_col = 0;
      } else {
        *(idx+1) = (val + ln_val) >> 1;
      }

      /* Do low nibble with indexes shifted 2 */

      val = ((((*(idx_behind+2)               // row-1, col-1
              + hn_val) >> 1)
              + *(idx_behind+4)) >> 1) // row-1, col+1
              + gstep[low_nibble];

      if (val < 0)
        val = 0;
      else if (val > 255)
        val = 255;

      /* Cache it for next loop before shifting */
      ln_val = val;

      *(idx+4) = val;
      *(idx+3) = (val + hn_val) >> 1;

      /* Same for the first line of the image */
      if (at_very_first_row) {
        /* row-1,col+1 / row-1,col+3*/
        *(idx_behind+6) = *(idx_behind+4) = val;
      }

      /* Shift indexes */
      idx_behind +=4;
      idx+=4;
    }
    *(idx+2) = val;
    at_very_first_row = 0;
  }
}
