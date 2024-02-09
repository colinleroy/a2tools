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
static uint8 *last_two_lines, *third_line;
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
uint8 *idx_min2;
uint8 *idx_end;
uint8 *idx_behind_plus2;
uint16 *idx_pix_rows;

uint8 val_col_minus2;


/* Internal data buffer
 * We don't use the destination raw_image directly,
 * because the algorithm uses two pixels bands top,
 * left, right and bottom as scratch. So we need a
 * 24x644 buffer to decode our 20x640 band. We add
 * two pixels on a 25th line because they are re-
 * used at the start of the next band.
 */
#define PIX_WIDTH 644
static uint8 pixelbuf[(QT_BAND + 4)*PIX_WIDTH + 2];
static uint8 *pix_direct_row[QT_BAND + 5];

void qt_load_raw(uint16 top)
{
  register uint8 *idx;
  register uint8 at_very_first_col;
  register uint8 row;
  register int16 val;

  /* First band: init variables */
  if (top == 0) {
    reset_bitbuff();

    at_very_first_row = 1;
    width_plus2 = width + 2;
    pgbar_state = 0;

    /* Init direct pointers to each line */
    for (row = 0; row < QT_BAND + 5; row++) {
      pix_direct_row[row] = pixelbuf + (row * PIX_WIDTH);
    }

    /* calculate offsets to shift the last two lines + 2px
     * from the end of the previous band to the start of
     * the new one.
     */
    last_two_lines = pix_direct_row[QT_BAND];
    third_line = pix_direct_row[2] + 2;

    /* Init the whole buffer with grey. */
    memset (pixelbuf, 0x80, sizeof pixelbuf);
  } else {
    /* Shift the last band's last 2 lines, plus 2 pixels,
     * to the start of the new band. */
    memcpy(pixelbuf, last_two_lines, 2*PIX_WIDTH + 2);
    /* And reset the others to grey */
    memset (third_line, 0x80, sizeof pixelbuf - (2*PIX_WIDTH + 2));
  }

  /* We start at line 2. */
  src = pix_direct_row[2];
  for (row = QT_BAND; row != 0; row--) {

    idx = src;
    if (row & 1) {
      idx_forward = src + PIX_WIDTH;
      idx_min2 = idx = src + 1;
      idx_end = src + width_plus2 + 1;
      pgbar_state+=2;
      progress_bar(-1, -1, 80*22, pgbar_state, height);
    } else {
      idx_min2 = idx = src;
      idx_end = src + width_plus2;
      idx_forward = src + PIX_WIDTH + 1;
    }

    val_col_minus2 = (*idx);
    idx += 2;

    /* row-1, col-1 */
    idx_behind = idx - (PIX_WIDTH+1);

    /* row-1, col+1 */
    idx_behind_plus2 = idx_behind + 2;

    /* Shift for next line */
    src += PIX_WIDTH;

    at_very_first_col = 1;

    while (idx != idx_end) {
      uint8 h = get_four_bits();

      val = ((*idx_behind               // row-1, col-1
              + (*(idx_behind_plus2) << 1) // row-1, col+1
              + val_col_minus2) >> 2)   // row  , col-2
              + gstep[h];

      if (val < 0)
        val = 0;
      else if (val & 0xff00) /* > 255, but faster as we're sure it's non-negative */
        val = 255;

      *(idx) = val;

      /* Cache it for next loop before shifting */
      val_col_minus2 = val;

      idx_behind = idx_behind_plus2;
      idx_behind_plus2+=2;

      if (at_very_first_col) {
        *(idx_forward) = *(idx_min2) = val;
        at_very_first_col = 0;
      }

      if (at_very_first_row) {
        /* row-1,col+1 / row-1,col+3*/
        *(idx_behind) = *(idx_behind_plus2) = val;
      }

      idx+=2;
    }
    *(idx) = val;
    at_very_first_row = 0;
  }

  /* Finish */
  src = pix_direct_row[2];
  //for (row = 2; row != QT_BAND + 2; row++) {
  for (row = QT_BAND; row != 0; row--) {
    if (row & 1) {
      idx_behind = src+1;
    } else {
      idx_behind = src+2;
    }
    idx = idx_behind+1;
    idx_end = idx + width;
    idx_forward = idx + 1;

    src += PIX_WIDTH;

    while (idx != idx_end) {
      val = ((*(idx_behind) // row,col-1
            + (*(idx) << 2) //row,col
            +  *(idx_forward)) >> 1) //row,col+1
            - 0x100;

      if (val < 0)
        *(idx) = 0;
      else if (val & 0xff00) /* > 255, but faster as we're sure it's non-negative */
        *(idx) = 255;
      else
        *(idx) = val;

      idx_behind = ++idx;
      idx_forward = ++idx;
      idx_forward++;
    }
  }

  dst = raw_image;
  src = pix_direct_row[2] + 2;
  for (row = 0; row < QT_BAND; row++) {
    memcpy(dst, src, width);
    dst+=width;
    src+=PIX_WIDTH;
  }
}
