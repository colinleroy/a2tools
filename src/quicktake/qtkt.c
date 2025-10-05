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

/* Shared with qt-conv.c */
char magic[5] = QTKT_MAGIC;
char *model = "100";
uint16 *huff_ptr = NULL; /* unused here, just for linking */
uint8 cache[CACHE_SIZE];
uint8 *cache_start = cache;

/* Pointer arithmetic helpers */
static uint16 pgbar_state;
static uint8 *last_two_lines;
static uint8 at_very_first_row;

/* Decoding stuff. The values from 0-7 are negative
 * but we reverse them when we use them for optimisation
 */
static const int16 gstep[16] =
{ -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };

/* Internal data buffer
 * We don't use the destination raw_image directly,
 * because the algorithm uses two pixels bands top,
 * left, right and bottom as scratch. So we need a
 * 24x644 buffer to decode our 20x640 band. We add
 * two pixels on a 25th line because they are re-
 * used at the start of the next band.
 */
#define SCRATCH_HEIGHT (BAND_HEIGHT + 1)
uint8 raw_image[(SCRATCH_HEIGHT-1) * RAW_WIDTH + 644];

size_t bytes_read = 0;
int full_reads, last_read;
uint8 next_ln_val;
static uint8 *idx = raw_image + (RAW_WIDTH);
#define idx_behind (idx-RAW_WIDTH+1)
void qt_load_raw(uint16 top)
{

  register uint8 at_very_first_col;
  register uint8 row;
  register int16 val;
  uint8 ln_val, hn_val;
  uint8 loop;

  /* First band: init variables */
  if (top == 0) {
    if (width == 640) {
      full_reads = ((640*480/4)/CACHE_SIZE);
      last_read = ((640*480/4)%CACHE_SIZE);
    } else {
      full_reads = ((320*240/4)/CACHE_SIZE);
      last_read = ((320*240/4)%CACHE_SIZE);
    }
    printf("Full reads needed = %d\n", full_reads);

    at_very_first_row = 1;
    pgbar_state = 0;

    /* calculate offsets to shift the last two lines + 2px
     * from the end of the previous band to the start of
     * the new one.
     */
    last_two_lines = raw_image + ((BAND_HEIGHT-1) * RAW_WIDTH);

    next_ln_val = 0x80;

    /* Init the second line + 1 bytes of buffer with grey. */
    memset (raw_image, 0x80, RAW_WIDTH + 1);
  } else {
    idx = raw_image + (RAW_WIDTH);
    /* Shift the last band's last line, plus 1 pixels,
     * to second line of the new band. */
    memcpy(raw_image, last_two_lines+RAW_WIDTH, RAW_WIDTH + 1);
  }

  /* In reality we do rows from 0 to BAND_HEIGHT, but decrementing is faster
   * and the only use of the variable is to check for oddity, so nothing
   * changes */
  for (row = BAND_HEIGHT; row != 0; row--) {
    uint16 x;
    /* Adapt indexes depending on the row's oddity */
    if (row & 1) {
      x = 1;
      pgbar_state+=2;
      progress_bar(-1, -1, 80*22, pgbar_state, height);
    } else {
      x = 0;
    }

    /* Initial set of the value two columns behind */
    ln_val = next_ln_val;

    at_very_first_col = 1;

    loop = width/4;
    /* First pass */
    while (1) {
      uint8 high_nibble, low_nibble;

      if (cur_cache_ptr == cache_end) {
        full_reads--;
        if (full_reads == 0) {
          printf("Doing small read %d\n", last_read);
          read(ifd, cur_cache_ptr = cache, last_read);
        } else {
          printf("Doing full read %d\n", CACHE_SIZE);
          read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
        }
      }

      high_nibble = *(cur_cache_ptr++);
      low_nibble = high_nibble & 0x0F;
      high_nibble >>= 4;

      /* Do high nibble */

      val = ((((idx_behind[x]               // row-1, col-1
              + ln_val) >> 1)
              + (idx_behind+2)[x]) >> 1) // row-1, col+1
              + gstep[high_nibble];

      if (val < 0)
        val = 0;
      else if (val > 255)
        val = 255;

      /* Cache it for next loop before shifting */
      hn_val = val;
      (idx+2)[x] = val;

      /* Same for the first line of the image */
      if (at_very_first_row) {
        /* row-1,col+1 / row-1,col+3*/
        (idx_behind+4)[x] = (idx_behind+2)[x] = val;
      }

      /* At first columns, we have to set scratch values for the next line.
       * We'll need them in the second pass */
      if (at_very_first_col) {
        next_ln_val = (idx+1)[x] = (idx)[x] = val;
        at_very_first_col = 0;
      } else {
        (idx+1)[x] = (val + ln_val) >> 1;
      }

      /* Do low nibble with indexes shifted 2 */
      val = (((((idx_behind+2)[x]               // row-1, col-1
              + hn_val) >> 1)
              + (idx_behind+4)[x]) >> 1) // row-1, col+1
              + gstep[low_nibble];

      if (val < 0)
        val = 0;
      else if (val > 255)
        val = 255;

      /* Cache it for next loop before shifting */
      ln_val = val;
      (idx+4)[x] = val;

      /* Same for the first line of the image */
      if (at_very_first_row) {
        /* row-1,col+1 / row-1,col+3*/
        (idx_behind+6)[x] = (idx_behind+4)[x] = val;
      }

      (idx+3)[x] = (ln_val + hn_val) >> 1;

      loop--;
      if (loop == 0) {
        break;
      }

      x += 4;
      /* Mimic Y register for clarity */
      if (x > 256) {
        x = x % 256;
        idx += 256;
      }
    }
    (idx+6)[x] = ln_val;

    if (width == 320) {
      idx += 512;
    } else {
      idx += 256;
    }

    at_very_first_row = 0;
    printf("total read %lu, pos in cache %lu\n", bytes_read, cur_cache_ptr-cache_start);
  }
}
