/*
   QTKN (QuickTake 150/200) decoding algorithm
   Copyright 2023, Colin Leroy-Mira <colin@colino.net>

   Heavily based on dcraw.c -- Dave Coffin's raw photo decoder
   Copyright 1997-2018 by Dave Coffin, dcoffin a cybercom o net

   Welcome to pointer arithmetic hell - you can refer to dcraw's
   kodak_radc_load_raw() is you prefer triple-dimensional array hell
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

#define radc_token(ch) ((int8) getbithuff(8,ch))

/* Shared with qt-conv.c */
char magic[5] = QTKN_MAGIC;
char *model = "150";
uint16 cache_size = 4096;
uint8 cache[4096];

static uint16 val_from_last[256];
static uint16 huff[19][256], *huff_9, *huff_10;
static int16 x, s, i, tree, tmp_i16;
static uint16 c, half_width, col;
static uint8 r, nreps, rep, row, y, t;
#define DATABUF_SIZE 386
static uint16 buf[3][DATABUF_SIZE], (*cur_buf)[DATABUF_SIZE];
static uint16 val;
static int8 tk;
static uint8 tmp8;
static uint16 tmp16;
static uint32 tmp32, tmp32_2;

#ifdef __CC65__
#define raw_ptr1 zp6p
#define raw_ptr2 zp8p
#define cur_buf_prevy zp10ip
#define cur_buf_y zp12ip
#else
static uint8 *raw_ptr1;
static uint8 *raw_ptr2;
static uint16 *cur_buf_prevy;
static uint16 *cur_buf_y;
#endif
static uint16 *huff_18;
static uint16 *cur_huff;
static uint16 *cur_buf_1;
static uint16 row_idx, row_idx_plus2, row_idx_shift;

static const int8 src[] = {
  1,1, 2,3, 3,4, 4,2, 5,7, 6,5, 7,6, 7,8,
  1,0, 2,1, 3,3, 4,4, 5,2, 6,7, 7,6, 8,5, 8,8,
  2,1, 2,3, 3,0, 3,2, 3,4, 4,6, 5,5, 6,7, 6,8,
  2,0, 2,1, 2,3, 3,2, 4,4, 5,6, 6,7, 7,5, 7,8,
  2,1, 2,4, 3,0, 3,2, 3,3, 4,7, 5,5, 6,6, 6,8,
  2,3, 3,1, 3,2, 3,4, 3,5, 3,6, 4,7, 5,0, 5,8,
  2,3, 2,6, 3,0, 3,1, 4,4, 4,5, 4,7, 5,2, 5,8,
  2,4, 2,7, 3,3, 3,6, 4,1, 4,2, 4,5, 5,0, 5,8,
  2,6, 3,1, 3,3, 3,5, 3,7, 3,8, 4,0, 5,2, 5,4,
  2,0, 2,1, 3,2, 3,3, 4,4, 4,5, 5,6, 5,7, 4,8,
  1,0, 2,2, 2,-2,
  1,-3, 1,3,
  2,-17, 2,-5, 2,5, 2,17,
  2,-7, 2,2, 2,9, 2,18,
  2,-18, 2,-9, 2,-2, 2,7,
  2,-28, 2,28, 3,-49, 3,-9, 3,9, 4,49, 5,-79, 5,79,
  2,-1, 2,13, 2,26, 3,39, 4,-16, 5,55, 6,-37, 6,76,
  2,-26, 2,-13, 2,1, 3,-39, 4,16, 5,-55, 6,-76, 6,37
};
static uint8 last = 16;
static uint16 *midbuf1, *midbuf2;

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

void qt_load_raw(uint16 top, uint8 h)
{
  register uint16 tmp16_2;
  register uint16 *cur_buf_x, *cur_buf_x_plus1;

  if (top == 0) {
    /* Init */
    for (s = i = 0; i < sizeof src; i += 2) {
      for (c=0; c < 256 >> src[i]; c++) {
        ((uint16 *)(huff))[s] = (src[i] << 8 | (uint8) src[i+1]);
        s++;
      }
    }

    for (c=0; c < 256; c++) {
      huff[18][c] = (1284 | c);
    }
    huff_9 = huff[9];
    huff_10 = huff[10];
    huff_18 = huff[18];
    getbits(0);

    cur_buf_y = buf[0];
    for (i=0; i < DATABUF_SIZE; i++) {
      *cur_buf_y = 2048;
      cur_buf_y++;
    }

    for (i = 1; i < 256; i++) {
      tmp32 = (0x1000000L/(uint32)i + 0x7ff);
      FAST_SHIFT_RIGHT_8_LONG(tmp32);
      val_from_last[i] = (tmp32 >> 4);
    }

    half_width = width / 2;
    row_idx_shift = width * 4;

    midbuf1 = &(buf[1][half_width]);
    midbuf2 = &(buf[2][half_width]);
  }

  row_idx = 0;
  row_idx_plus2 = width * 2;

  for (row=0; row < h; row+=4) {
    progress_bar(-1, -1, 80*22, (top + row), height);
    t = getbits(6);
    /* Ignore those */
    getbits(6);
    getbits(6);

    cur_buf = buf;
    c = 0;

    val = val_from_last[last];
    val *= t;

    cur_buf_y = cur_buf[0];
    cur_buf_1 = cur_buf[1];

#ifndef __CC65__
    tmp32_2 = (uint32)val;
#else
    __asm__("lda %v", val);
    __asm__("sta %v", tmp32_2);
    __asm__("lda %v+1", val);
    __asm__("sta %v+1", tmp32_2);
    __asm__("stz %v+2", tmp32_2);
    __asm__("stz %v+3", tmp32_2);
#endif

    for (i=0; i != DATABUF_SIZE; i++) {
#ifndef __CC65__
      tmp32 = tmp32_2;
      tmp32 *= (*cur_buf_y);
      tmp32--;
      *((uint16 *)cur_buf_y) = tmp32 >> 12;
#else
    /* load */
    __asm__("lda %v+3", tmp32_2);
    __asm__("sta sreg+1");
    __asm__("lda %v+2", tmp32_2);
    __asm__("sta sreg");
    __asm__("ldx %v+1", tmp32_2);
    __asm__("lda %v", tmp32_2);
    /* multiply */
    __asm__("jsr pusheax");
    __asm__("ldy #$01");
    __asm__("lda (%v),y", cur_buf_y);
    __asm__("tax");
    __asm__("lda (%v)", cur_buf_y);
    __asm__("jsr tosumul0ax");
    /* store */
    __asm__("sta %v", tmp32);
    __asm__("stx %v+1", tmp32);
    __asm__("ldy sreg");
    __asm__("sty %v+2", tmp32);
    __asm__("ldy sreg+1");
    __asm__("sty %v+3", tmp32);
    /* decrement */
    tmp32--;
    /* Shift >> 12 */
    /* First 8 */
    __asm__("stz sreg+1");
    __asm__("lda %v+3", tmp32);
    __asm__("sta sreg");
    __asm__("ldx %v+2", tmp32);
    __asm__("lda %v+1", tmp32);

    /* Then 4 */
    __asm__("jsr shreax4");
    __asm__("sta (%v)", cur_buf_y);
    __asm__("ldy #$01");
    __asm__("txa");
    __asm__("sta (%v),y", cur_buf_y);
#endif

      cur_buf_y++;
    }

    last = t;

    for (r=0; r != 2; r++) {
      tree = t << 7;
     *midbuf1 = tree;
     *midbuf2 = tree;
      for (tree = 1, col = half_width; col; ) {
        cur_huff = huff[tree];
        if ((tree = (int16)radc_token(cur_huff))) {
          col -= 2;
          if (tree == 8) {
            x = col + 1;
            cur_buf_x = cur_buf_1 + x;
            for (y=1; y < 3; y++) {
              *cur_buf_x = (uint8) radc_token(huff_18) * t;
              cur_buf_x--;
              *cur_buf_x = (uint8) radc_token(huff_18) * t;
              cur_buf_x += DATABUF_SIZE + 1;
            }
          } else {
            uint16 *tmp_prevy;
            cur_buf_prevy = cur_buf[0] + (col + 1);
            cur_buf_x = cur_buf_prevy + DATABUF_SIZE;
            cur_buf_x_plus1 = cur_buf_x + 1;
            cur_huff = (uint16 *)(huff + tree + 10);
            for (y=1; ; y++) {
              /* Unrolled */
              tk = radc_token(cur_huff);
              *cur_buf_x = tk * 16;

#ifndef __CC65__
              tmp16_2 = *cur_buf_prevy << 1;
              tmp_prevy = cur_buf_prevy++;
              tmp16_2 += *cur_buf_prevy
                         + *cur_buf_x_plus1;
              tmp16_2 >>= 2;
              *cur_buf_x += tmp16_2;
#else
              __asm__("ldy #$01");
              __asm__("lda (%v),y", cur_buf_prevy);
              __asm__("sta tmp1");
              __asm__("lda (%v)", cur_buf_prevy);
              __asm__("asl a");
              __asm__("rol tmp1");
              __asm__("ldx tmp1");
              __asm__("sta %v", tmp16_2);
              __asm__("stx %v+1", tmp16_2);

              tmp_prevy = cur_buf_prevy;

              /* Inc (twice because uint16) */
              __asm__("inc %v", cur_buf_prevy);
              __asm__("bne %g", noof2);
              __asm__("inc %v+1", cur_buf_prevy);
              noof2:
              __asm__("inc %v", cur_buf_prevy);
              __asm__("bne %g", noof3);
              __asm__("inc %v+1", cur_buf_prevy);
              noof3:
              __asm__("clc");
              __asm__("lda (%v)", cur_buf_prevy);
              __asm__("adc (%v)", cur_buf_x_plus1);
              __asm__("tax");
              __asm__("ldy #$01");
              __asm__("lda (%v),y", cur_buf_prevy);
              __asm__("adc (%v),y", cur_buf_x_plus1);
              __asm__("tay");

              __asm__("txa");
              __asm__("clc");
              __asm__("adc %v", tmp16_2);
              __asm__("pha");
              __asm__("tya");
              __asm__("adc %v+1", tmp16_2);
              __asm__("tax");
              __asm__("pla");
              __asm__("jsr shrax2");

              __asm__("clc");
              __asm__("adc (%v)", cur_buf_x);
              __asm__("sta (%v)", cur_buf_x);

              __asm__("txa");
              __asm__("ldy #$01");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);

#endif

              cur_buf_x_plus1 = cur_buf_x--;

              cur_buf_prevy = tmp_prevy;
              /* Second with col - 1*/
              tk = radc_token(cur_huff);
              *cur_buf_x = tk * 16;

#ifndef __CC65__
              tmp16_2 = *cur_buf_prevy;
              cur_buf_prevy--;
              tmp16_2 += (*cur_buf_prevy << 1)
                         + *cur_buf_x_plus1;
              tmp16_2 >>= 2;
              *cur_buf_x += tmp16_2;
#else
              __asm__("ldy #$01");
              __asm__("lda (%v),y", cur_buf_prevy);
              __asm__("sta %v+1", tmp16_2);
              __asm__("lda (%v)", cur_buf_prevy);
              __asm__("sta %v", tmp16_2);

              /* Dec cur_buf_prevy */
              __asm__("lda %v", cur_buf_prevy);
              __asm__("sec");
              __asm__("sbc #$02");
              __asm__("sta %v", cur_buf_prevy);
              __asm__("bcs %g", nouf1);
              __asm__("dec %v+1", cur_buf_prevy);
              nouf1:
              /* *cur_buf_prevy << 1 */
              __asm__("lda (%v),y", cur_buf_prevy);
              __asm__("sta tmp1");
              __asm__("lda (%v)", cur_buf_prevy);
              __asm__("asl a");
              __asm__("rol tmp1");

              /* Add tmp16_2 */
              __asm__("clc");
              __asm__("adc %v", tmp16_2);
              __asm__("tay");
              __asm__("lda tmp1"); /* *cur_buf_prevy<<1 in AX */
              __asm__("adc %v+1", tmp16_2);
              __asm__("tax");
              __asm__("tya"); /* cur_buf_prevy<<1 + tmp16_2 in AX */
              /* Add *cur_buf_x_plus1 */

              __asm__("ldy #$01");
              __asm__("clc");
              __asm__("adc (%v)", cur_buf_x_plus1);
              __asm__("pha");
              __asm__("txa");
              __asm__("adc (%v),y", cur_buf_x_plus1);
              __asm__("tax");
              __asm__("pla"); /* cur_buf_prevy<<1 + tmp16_2 + cur_buf_xplus1 in AX */
              __asm__("jsr shrax2"); /* (cur_buf_prevy<<1 + tmp16_2 + cur_buf_xplus1)>>2 in AX */
              /* Store to cur_buf_x */
              __asm__("clc");
              __asm__("adc (%v)", cur_buf_x);
              __asm__("sta (%v)", cur_buf_x);

              __asm__("txa");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);
#endif
              if (y == 2)
                break;
              cur_buf_x += DATABUF_SIZE+1;
              cur_buf_x_plus1 += DATABUF_SIZE+1;
              cur_buf_prevy += DATABUF_SIZE+1;
            }
          }
        } else {
          do {
            nreps = (col > 2) ? radc_token(huff_9) + 1 : 1;
            for (rep=0; rep < 8 && rep < nreps && col; rep++) {
              uint16 *tmp_prevy;
              col -= 2;
              tmp16 = col + 1;
              cur_buf_prevy = cur_buf[0] + tmp16;
              cur_buf_x = cur_buf_prevy + DATABUF_SIZE;
              cur_buf_x_plus1 = cur_buf_x + 1;
              for (y=1; ; y++) {
                /* Unrolled */

#ifndef __CC65__
                *cur_buf_x = *cur_buf_prevy << 1;
                tmp_prevy = cur_buf_prevy++;
                *cur_buf_x += *cur_buf_prevy
                           + *cur_buf_x_plus1;
                *cur_buf_x >>= 2;
#else
              __asm__("ldy #$01");
              __asm__("lda (%v),y", cur_buf_prevy);
              __asm__("sta tmp1");
              __asm__("lda (%v)", cur_buf_prevy);
              __asm__("asl a");
              __asm__("rol tmp1");
              __asm__("ldx tmp1");
              __asm__("sta (%v)", cur_buf_x);
              __asm__("txa");
              __asm__("sta (%v),y", cur_buf_x);

              tmp_prevy = cur_buf_prevy;

              /* Inc (twice because uint16) */
              __asm__("inc %v", cur_buf_prevy);
              __asm__("bne %g", noof4);
              __asm__("inc %v+1", cur_buf_prevy);
              noof4:
              __asm__("inc %v", cur_buf_prevy);
              __asm__("bne %g", noof5);
              __asm__("inc %v+1", cur_buf_prevy);
              noof5:
              __asm__("clc");
              __asm__("lda (%v)", cur_buf_prevy);
              __asm__("adc (%v)", cur_buf_x_plus1);
              __asm__("tax");
              __asm__("ldy #$01");
              __asm__("lda (%v),y", cur_buf_prevy);
              __asm__("adc (%v),y", cur_buf_x_plus1);
              __asm__("tay");

              __asm__("clc");
              __asm__("txa");
              __asm__("adc (%v)", cur_buf_x);
              __asm__("pha");
              __asm__("tya");
              __asm__("ldy #$01");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("tax");
              __asm__("pla");
              __asm__("jsr shrax2");
              __asm__("sta (%v)", cur_buf_x);
              __asm__("txa");
              __asm__("sta (%v),y", cur_buf_x);
#endif
                cur_buf_x_plus1 = cur_buf_x;
                cur_buf_x--;
                cur_buf_prevy = --tmp_prevy;
                /* Second */
                if (c) {
#ifndef __CC65__
                  *cur_buf_x = (*cur_buf_prevy 
                                + *(cur_buf_x_plus1)) << 1;
#else
                  __asm__("clc");
                  __asm__("ldy #$01");
                  __asm__("lda (%v)", cur_buf_prevy);
                  __asm__("adc (%v)", cur_buf_x_plus1);
                  __asm__("pha");
                  __asm__("lda (%v),y", cur_buf_prevy);
                  __asm__("adc (%v),y", cur_buf_x_plus1);
                  __asm__("sta tmp1");
                  __asm__("pla");
                  __asm__("asl a");
                  __asm__("rol tmp1");
                  __asm__("sta (%v)", cur_buf_x);
                  __asm__("lda tmp1");
                  __asm__("sta (%v),y", cur_buf_x);
#endif
                } else {
#ifndef __CC65__
                  *cur_buf_x = *cur_buf_prevy << 1;
                  tmp_prevy = cur_buf_prevy++;
                  *cur_buf_x += *cur_buf_prevy
                             + *cur_buf_x_plus1;
                  *cur_buf_x >>= 2;
#else
                  __asm__("ldy #$01");
                  __asm__("lda (%v),y", cur_buf_prevy);
                  __asm__("sta tmp1");
                  __asm__("lda (%v)", cur_buf_prevy);
                  __asm__("asl a");
                  __asm__("rol tmp1");
                  __asm__("ldx tmp1");
                  __asm__("sta (%v)", cur_buf_x);
                  __asm__("txa");
                  __asm__("sta (%v),y", cur_buf_x);

                  tmp_prevy = cur_buf_prevy;

                  /* Inc (twice because uint16) */
                  __asm__("inc %v", cur_buf_prevy);
                  __asm__("bne %g", noof8);
                  __asm__("inc %v+1", cur_buf_prevy);
                  noof8:
                  __asm__("inc %v", cur_buf_prevy);
                  __asm__("bne %g", noof9);
                  __asm__("inc %v+1", cur_buf_prevy);
                  noof9:
                  __asm__("clc");
                  __asm__("lda (%v)", cur_buf_prevy);
                  __asm__("adc (%v)", cur_buf_x_plus1);
                  __asm__("tax");
                  __asm__("ldy #$01");
                  __asm__("lda (%v),y", cur_buf_prevy);
                  __asm__("adc (%v),y", cur_buf_x_plus1);
                  __asm__("tay");

                  __asm__("clc");
                  __asm__("txa");
                  __asm__("adc (%v)", cur_buf_x);
                  __asm__("pha");
                  __asm__("tya");
                  __asm__("ldy #$01");
                  __asm__("adc (%v),y", cur_buf_x);
                  __asm__("tax");
                  __asm__("pla");
                  __asm__("jsr shrax2");
                  __asm__("sta (%v)", cur_buf_x);
                  __asm__("txa");
                  __asm__("sta (%v),y", cur_buf_x);
#endif
                  cur_buf_prevy = tmp_prevy;
                }
                if (y == 2)
                  break;
                cur_buf_x += DATABUF_SIZE+1;
                cur_buf_x_plus1 += DATABUF_SIZE+1;
                cur_buf_prevy += DATABUF_SIZE+1;
              }
              if (rep & 1) {
                tk = radc_token(huff_10);
                tmp_i16 = tk << 4;

                tmp16 = col + 1;
                cur_buf_prevy = cur_buf[0] + tmp16;
                cur_buf_x = cur_buf_prevy + DATABUF_SIZE;

                for (y=1; ; y++) {
                  /* Unrolled */
#ifndef __CC65__
                  *cur_buf_x += tmp_i16;
#else
                  __asm__("ldy #$01");
                  __asm__("clc");
                  __asm__("lda %v", tmp_i16);
                  __asm__("adc (%v)", cur_buf_x);
                  __asm__("sta (%v)", cur_buf_x);
                  __asm__("lda %v+1", tmp_i16);
                  __asm__("adc (%v),y", cur_buf_x);
                  __asm__("sta (%v),y", cur_buf_x);
#endif
                  cur_buf_x_plus1 = cur_buf_x;
                  cur_buf_x--;
                  cur_buf_prevy--;
#ifndef __CC65__
                  *cur_buf_x += tmp_i16;
#else
                  __asm__("ldy #$01");
                  __asm__("clc");
                  __asm__("lda %v", tmp_i16);
                  __asm__("adc (%v)", cur_buf_x);
                  __asm__("sta (%v)", cur_buf_x);
                  __asm__("lda %v+1", tmp_i16);
                  __asm__("adc (%v),y", cur_buf_x);
                  __asm__("sta (%v),y", cur_buf_x);
#endif
                  if (y == 2)
                    break;
                  cur_buf_x += DATABUF_SIZE+1;
                  cur_buf_x_plus1 += DATABUF_SIZE+1;
                  cur_buf_prevy += DATABUF_SIZE+1;
                }
              }
            }
          } while (nreps == 9);
        }
      }
      raw_ptr1 = raw_image + row_idx; //FILE_IDX(row, 0);
      raw_ptr2 = raw_image + row_idx_plus2; //FILE_IDX(row + 2, 0);
      cur_buf_y = cur_buf_1;
      for (y=1; ; y++) {
        cur_buf_x = cur_buf_y;
        for (x=0; x < half_width; x++) {
          val = (*cur_buf_x) / t;
          cur_buf_x++;
          if (val > 255)
            val = 255;
          if (r) {
            *raw_ptr2 = val;
          } else {
            *raw_ptr1 = val;
          }
          raw_ptr1 += 2;
          raw_ptr2 += 2;
        }
        if (y == 2)
          break;
        raw_ptr1++;
        raw_ptr2++;
        cur_buf_y += DATABUF_SIZE;
      }
      memcpy (cur_buf[0]+1, cur_buf[2], sizeof cur_buf[0]-2);
    }
    cur_buf += 3;

    /* Consume RADC tokens but don't care about them. */
    for (c=1; c < 3; c++) {
      tree = t << 7;
      for (tree = 1, col = half_width; col; ) {
        cur_huff = huff[tree];
        if ((tree = (int16)radc_token(cur_huff))) {
          col -= 2;
          if (tree == 8) {
            radc_token(huff_18);
            radc_token(huff_18);
            radc_token(huff_18);
            radc_token(huff_18);
          } else {
            tmp_i16 = tree + 10;
            cur_huff = huff[tmp_i16];
            for (y=1; y < 3; y++) {
              radc_token(cur_huff);
              radc_token(cur_huff);
            }
          }
        } else {
          do {
            nreps = (col > 2) ? radc_token(huff_9) + 1 : 1;
            for (rep=0; rep < 8 && rep < nreps && col; rep++) {
              col -= 2;
              if (rep & 1) {
                radc_token(huff_10);
              }
            }
          } while (nreps == 9);
        }
      }
    }

    raw_ptr1 = raw_image + row_idx - 1;
    raw_ptr2 = raw_ptr1 + 1;
    for (y = 4; y; y--) {
      tmp8 = y & 1;
      if (!tmp8) {
        raw_ptr1++;
        raw_ptr2++;
        x = 0;
      } else {
        x = 1;
      }
      for (; x < width; x+=2) {
        *raw_ptr2 = *raw_ptr1;
        raw_ptr1++;
        raw_ptr1++;
        raw_ptr2++;
        raw_ptr2++;
      }
      if (!tmp8) {
        raw_ptr1--;
        raw_ptr2--;
      }
    }

    row_idx += row_idx_shift;
    row_idx_plus2 += row_idx_shift;
  }
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
