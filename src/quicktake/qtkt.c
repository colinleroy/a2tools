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
uint16 *huff_ptr = NULL; /* unused here, just for linking */
uint8 *dst, *src;

/* Pointer arithmetic helpers */
static uint16 width_plus2;
static uint16 pgbar_state;
static uint16 twopxband_size, work_size;
static uint8 *last_two_lines, *third_line;
static uint8 at_very_first_line;

/* Decoding stuff. The values from 0-7 are negative
 * but we reverse them when we use them for optimisation 
 */
static const uint8 gstep[16] =
{ 89,60,44,32,22,15,8,2,2,8,15,22,32,44,60,89 };
#define NEG_STEPS 7

/* Indexes. We need to follow the current pixel (at idx) at
 * various places: 
 * idx_forward is start of the next row,
 * idx_behind is row-1, col-1,
 * idx_behind_plus2 is row-1, col-3,
 * idx_end is where we stop to finish a line
 */
#ifdef __CC65__
#define idx_forward zp6p
#define idx_behind zp8p
#define idx_end zp10p
#define idx_behind_plus2 zp12p
#else
uint8 *idx_forward;
uint8 *idx_behind;
uint8 *idx_end;
uint8 *idx_behind_plus2;
#endif
uint8 val_col_minus2, val_col_minus2_save;

/* Reuse same vars in the second loop.
 * idx_min1 is row,col-1,
 * idx_plus1 is row,col+1
 */
#define idx_min1 idx_behind
#define idx_plus1 idx_forward


/* Internal data buffer
 * We don't use the destination raw_image directly,
 * because the algorithm uses two pixels bands top,
 * left, right and bottom as scratch. So we need a
 * 24x644 buffer to decode our 20x640 band. We add
 * two pixels on a 25th line because they are re-
 * used at the start of the next band.
 */
#define PIX_WIDTH 644
static uint8 pixel[(QT_BAND + 4)*PIX_WIDTH + 2];
static uint8 *pix_direct_row[QT_BAND + 5];

void qt_load_raw(uint16 top)
{
  register uint8 *idx;
  register uint8 at_very_first_col;
  register uint8 row;
  register int16 val;

  /* First band: init variables */
  if (top == 0) {
    getbits(0);

    at_very_first_line = 1;
    width_plus2 = width + 2;
    pgbar_state = 0;

    /* Init direct pointers to each line */
    for (row = 0; row < QT_BAND + 5; row++) {
      pix_direct_row[row] = pixel + (row * PIX_WIDTH);
    }

    /* calculate offsets to shift the last two lines + 2px 
     * from the end of the previous band to the start of
     * the new one.
     */
    twopxband_size = 2*PIX_WIDTH + 2;
    work_size = sizeof pixel - twopxband_size;
    last_two_lines = pix_direct_row[QT_BAND];
    third_line = pix_direct_row[2] + 2;

    /* Init the whole buffer with grey. */
    memset (pixel, 0x80, sizeof pixel);
  } else {
    /* Shift the last band's last 2 lines, plus 2 pixels,
     * to the start of the new band. */
    memcpy(pixel, last_two_lines, twopxband_size);
    /* And reset the others to grey */
    memset (third_line, 0x80, work_size);
  }

  /* We start at line 2. */
  src = pix_direct_row[2];
  for (row = 2; row != QT_BAND + 2; row++) {
    int cnt = 0;
    idx_forward = idx_end = idx = src;
    if (row & 1) {
      idx++;
      pgbar_state++;
      pgbar_state++;
      progress_bar(-1, -1, 80*22, pgbar_state, height);
    } else {
      idx_forward++;
    }
    val_col_minus2 = (*idx);
    idx++;
    idx++;

    idx_behind = idx;

    /* row-1, col-1 */
    idx_behind -= (PIX_WIDTH+1);
    /* row-1, col+1 */
    idx_behind_plus2 = idx_behind + 2;
    /* row+1, ~row & 1 */
    idx_forward += PIX_WIDTH;

    /* Shift for next line */
    src += PIX_WIDTH;

    /* Width to decode: image width, not scratch buffer width */
    idx_end += width_plus2;

    at_very_first_col = 1;

    while (idx < idx_end) {
#ifndef __CC65__
      uint8 h = getbitnohuff(4);
      if (h > NEG_STEPS) {
        val = gstep[h];
      } else {
        val = -gstep[h];
      }
      val += ((*idx_behind             // row-1, col-1
              + (*(idx_behind_plus2))*2 // row-1, col+1
              + val_col_minus2) >> 2);  // row  , col-2
      if (val < 0)
        val = 0;
      else if (val & 0xff00) /* > 255, but faster as we're sure it's non-negative */
        val = 255;

      *(idx) = val;

      /* Cache it for next loop before shifting */
      val_col_minus2 = val;
#else
      __asm__("lda #4");
      __asm__("jsr %v", getbitnohuff);
      __asm__("tay");

      /* if h > NEG_STEPS */
      __asm__("cmp #%b", NEG_STEPS + 1);
      __asm__("bcc %g", h_neg);
      /* h is positive */
      __asm__("ldx #$00");
      __asm__("lda %v,y", gstep);
      __asm__("bra %g", gstep_done);
      h_neg:
      __asm__("lda %v,y", gstep);
      __asm__("ldx #$FF");
      __asm__("eor #$FF");
      __asm__("clc");
      __asm__("adc #$01");
      __asm__("bcc %g", gstep_done);
      __asm__("inx");
      gstep_done:
      __asm__("sta %v", val);
      __asm__("stx %v+1", val);

      __asm__("ldx #$00");

      /* *idx_behind_plus2 * 2 */
      __asm__("lda (%v)", idx_behind_plus2);
      __asm__("asl a");
      __asm__("bcc %g", noof1);
      __asm__("inx");
      __asm__("clc");

      noof1:
      /* *idx_behind */
      __asm__("adc (%v)", idx_behind);
      __asm__("bcc %g", noof2);
      __asm__("inx");
      __asm__("clc");

      noof2:
      /* val_col_minus2 */
      __asm__("adc %v", val_col_minus2);
      __asm__("bcc %g", noof3);
      __asm__("inx");
      noof3:

      /* >> 2 */
      __asm__("stx tmp1");
      __asm__("cpx #$80");
      __asm__("ror tmp1");
      __asm__("ror a");
      __asm__("cpx #$80");
      __asm__("ror tmp1");
      __asm__("ror a");
      __asm__("ldx tmp1");

      /* add to val */
      __asm__("clc");
      __asm__("adc %v", val);
      __asm__("tay"); /* Backup A */
      __asm__("txa");
      __asm__("adc %v+1", val);
      __asm__("sta %v+1", val);
      __asm__("tax"); /* Put high byte back where it belongs */
      __asm__("tya"); /* Restore A */

      /* val < 0 ? */
      __asm__("cpx #$80");
      __asm__("bcs %g", setzero);
      /* > 255 ? */
      __asm__("cpx #$00");
      __asm__("beq %g", done);
      __asm__("lda #$FF");
      __asm__("bra %g", done);
      setzero:
      __asm__("lda #$00");
      done:
      __asm__("stz %v+1", val);

      __asm__("sta %v", val);   /* set val */
      __asm__("sta (%v)", idx);
      __asm__("sta %v", val_col_minus2);

#endif
      idx_behind = idx_behind_plus2++;
      idx_behind_plus2++;

      if (at_very_first_col) {
        *(idx_forward) = *(idx - 2) = val;
        at_very_first_col = 0;
      }

      if (at_very_first_line) {
        /* row-1,col+1 / row-1,col+3*/
        *(idx_behind) = *(idx_behind_plus2) = val;
      }

      idx++;
      idx++;
    }

    *(idx) = val;
    at_very_first_line = 0;
  }

  src = pix_direct_row[2];
  for (row = 2; row != QT_BAND + 2; row++) {
    idx_end = idx = src;

    idx++;
    if (row & 1) {
      idx_min1 = idx++;
    } else {
      idx++;
      idx_min1 = idx++;
    }
    idx_plus1 = idx + 1;
    idx_end += width_plus2;
    src += PIX_WIDTH;

    while (idx < idx_end) {
#ifndef __CC65__
      val = ((*(idx_min1) // row,col-1
            + (*(idx) << 2) //row,col
            +  *(idx_plus1)) >> 1) //row,col+1
            - 0x100;

      if (val < 0)
        *(idx) = 0;
      else if (val & 0xff00) /* > 255, but faster as we're sure it's non-negative */
        *(idx) = 255;
      else
        *(idx) = val;

      idx_min1 = ++idx;
      idx_plus1 = ++idx;
      idx_plus1++;
#else
      /* *idx << 2 */
      __asm__("ldx #$00");
      __asm__("lda (%v)", idx);
      __asm__("stx tmp1");
      __asm__("asl a");
      __asm__("rol tmp1");
      __asm__("asl a");
      __asm__("rol tmp1");
      __asm__("ldx tmp1");

      /* + idx_min1 */
      __asm__("clc");
      __asm__("adc (%v)", idx_min1);
      __asm__("bcc %g", noof4);
      __asm__("inx");
      noof4:

      /* +idx_plus1 */
      __asm__("clc");
      __asm__("adc (%v)", idx_plus1);
      __asm__("bcc %g", noof5);
      __asm__("inx");
      noof5:

      /* >> 1 */
      __asm__("stx tmp1");
      __asm__("clc");
      __asm__("ror tmp1");
      __asm__("ror a");
      __asm__("ldx tmp1");

      /* - 0x100 */
      __asm__("dex");

      /* < 0 ? */
      __asm__("cpx #$80");
      __asm__("bcs %g", setzero2);
      /* > 255 ? */
      __asm__("cpx #$00");
      __asm__("beq %g", done2);
      __asm__("lda #$FF");
      __asm__("bra %g", done2);
      setzero2:
      __asm__("lda #$00");
      done2:
      __asm__("sta (%v)", idx); /* set idx */

      /* shift indexes */
      __asm__("ldx %v+1", idx);
      __asm__("lda %v", idx);
      __asm__("ina");
      __asm__("bne %g", noof6);
      __asm__("inx");
      noof6:
      __asm__("stx %v+1", idx_min1);
      __asm__("sta %v", idx_min1);
      __asm__("ina");
      __asm__("bne %g", noof7);
      __asm__("inx");
      noof7:
      __asm__("stx %v+1", idx);
      __asm__("sta %v", idx);
      __asm__("ina");
      __asm__("bne %g", noof8);
      __asm__("inx");
      noof8:
      __asm__("stx %v+1", idx_plus1);
      __asm__("sta %v", idx_plus1);
#endif
    }
    idx++;
    idx++;
  }

  dst = raw_image;
  src = pix_direct_row[2] + 2;
  for (row = 0; row < QT_BAND; row++) {
    memcpy(dst, src, width);
    dst+=width;
    src+=PIX_WIDTH;
  }
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
