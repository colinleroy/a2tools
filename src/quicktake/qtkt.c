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

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

/* Shared with qt-conv.c */
char magic[5] = QTKT_MAGIC;
char *model = "100";
uint16 cache_size = 4096;
uint8 cache[4096];

/* bitbuff state at end of first loop */
static uint32 prev_bitbuf_a = 0;
static uint8 prev_vbits_a = 0;
static uint32 prev_offset_a = 0;

/* Pointer arithmetic helpers */
static uint8 h_plus1, h_plus2, h_plus4;
static uint16 width_plus2, width_plus1;
static uint16 pgbar_state;
static uint16 threepxband_size, work_size;
static uint16 band_size;
static uint8 *last_two_lines, *third_line;
static uint8 at_very_first_line;

static const int8 gstep[16] =
{ -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
static int16 val;
static uint8 val_col_minus2;
static uint8 row;
static uint8 *src;
static uint8 *idx_forward, *idx_behind;

static uint8 *pix_direct_row[QT_BAND+5];

/* Internal buffer */
#define PIX_WIDTH 644
static uint8 pixel[(QT_BAND+5)*PIX_WIDTH];

void qt_load_raw(uint16 top, uint8 h)
{
  register uint8 *dst, *idx;
  register uint16 col;

  if (top == 0) {
    getbits(0);

    for (col = 0; col < QT_BAND + 5; col++) {
      pix_direct_row[col] = pixel + (width * col);
    }

    at_very_first_line = 1;
    h_plus1 = h + 1;
    h_plus2 = h + 2;
    h_plus4 = h + 4;
    width_plus2 = width + 2;
    width_plus1 = width + 1;
    pgbar_state = 0;
    band_size = h * width;
    /* calculate offsets to shift the last lines */
    threepxband_size = 3*width;
    work_size = sizeof pixel - threepxband_size;
    last_two_lines = pix_direct_row[QT_BAND + 2];
    third_line = pix_direct_row[3];
    memset (pixel, 0x80, sizeof pixel);
  } else {
    /* Shift last pixel lines to start */
    memcpy(pixel, last_two_lines, threepxband_size);
    /* And reset the others */
    memset (third_line, 0x80, work_size);

    /* Reset bitbuf where it should be */
    bitbuf = prev_bitbuf_a;
    vbits = prev_vbits_a;
    src_file_seek(prev_offset_a);
  }

  for (row=2; row < h_plus4; row++) {

    if (row & 1) {
      col = 3;
      if (row < h_plus2) {
        pgbar_state += 2;
        progress_bar(-1, -1, 80*22, pgbar_state, height);
      }
    } else {
      col = 2;
    }
    idx = pix_direct_row[row];
    idx += col;
    idx_behind = idx;
    idx_forward = idx;

    idx_behind -= width_plus1;
    idx_forward += width;

    val_col_minus2 = *(idx - 2);
    for (; col < width_plus2; col+=2) {
      val = ((*(idx_behind)           // row-1, col-1
              + (*(idx_behind + 2))*2 // row-1, col+1
              + val_col_minus2) >> 2) // row  , col-2
             + gstep[getbits(4)];

      if (val < 0)
        val = 0;
      else if (val > 255)
        val = 255;

      *(idx) = val;

      /* Cache it for next loop before shifting */
      val_col_minus2 = val;

      if (col < 4) {
        /* row, col-2 */
        *(idx - 2) = *(idx_forward + (~row & 1)) = val;
        idx_forward += 2;
      }

      idx += 2;
      idx_behind += 2;

      if (at_very_first_line) {
        /* row-1,col+1 / row-1,col+3*/
        *(idx_behind) = *(idx_behind + 2) = val;
      }
    }

    *(idx) = val;

    if(row == h_plus1) {
      /* Save bitbuf state at end of first loop */
      prev_bitbuf_a = bitbuf;
      prev_vbits_a = vbits;
      prev_offset_a = cache_read_since_inval();
    }
    at_very_first_line = 0;
  }

  for (row = 2; row < h_plus2; row++) {
    if (row & 1)
      col = 2;
    else
      col = 3;
    idx = pix_direct_row[row] + col;

    for (; col < width_plus2; col+=2) {
      val = ((*(idx-1) // row,col-1
            + (*(idx) << 2) //row,col
            +  *(idx+1)) >> 1) //row,col+1
            - 0x100;

      if (val < 0)
        *(idx) = 0;
      else if (val > 255)
        *(idx) = 255;
      else
        *(idx) = val;

      idx += 2;
    }
  }

  /* Move from tmp pixel array to raw_image */
  src = pix_direct_row[2] + 2;
  dst = raw_image;
  memcpy(dst, src, band_size);
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
