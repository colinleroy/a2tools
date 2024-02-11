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
#include "qtk_bithuff.h"

#define radc_token(r, t, ch) do { huff_ptr = ch; r = (t) getbithuff(8); } while(0)
#define skip_radc_token(ch) do { huff_ptr = ch; getbithuff(8); } while(0)

/* Shared with qt-conv.c */
char magic[5] = QTKN_MAGIC;
char *model = "150";
uint16 *huff_ptr;
uint8 cache[CACHE_SIZE];
uint8 *cache_start = cache;

uint8 got_four_bits; /* HACK to build */
static uint16 val_from_last[256];
static uint16 huff[19][256], *huff_9, *huff_10;
static int16 x, s, i, tmp_i16, tree;
static uint16 c, half_width, col, stop;
static uint8 r, nreps, rep, row, y, t, rep_loop;
#define DATABUF_SIZE 386
static uint16 buf[3][DATABUF_SIZE], (*cur_buf)[DATABUF_SIZE];
static uint16 val;
static int8 tk;
#ifndef __CC65__
static uint8 tmp8;
static uint16 tmp16;
#endif
static uint32 tmp32;

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

void qt_load_raw(uint16 top)
{
  register uint16 tmp16_2;
  register uint16 *cur_buf_x, *cur_buf_x_plus1;

  if (top == 0) {
    /* Init */
    for (s = i = 0; i != sizeof src; i += 2) {
      r = src[i];
      t = 256 >> r;
      y = src[i+1];
      do {
        ((uint16 *)(huff))[s] = (r << 8 | (uint8) y);
        s++;
      } while (--t);
    }

    for (c=0; c != 256; c++) {
      huff[18][c] = (1284 | c);
    }
    huff_9 = huff[9];
    huff_10 = huff[10];
    huff_18 = huff[18];

    cur_buf_y = buf[0];
    for (i=0; i != DATABUF_SIZE; i++) {
      *cur_buf_y = 2048;
      cur_buf_y++;
    }

    for (i = 1; i != 256; i++) {
      tmp32 = (0x1000000L/(uint32)i + 0x7ff);
      FAST_SHIFT_RIGHT_8_LONG(tmp32);
      val_from_last[i] = (tmp32 >> 4);
    }

    half_width = width >> 1;
    row_idx_shift = width << 2;

    midbuf1 = &(buf[1][half_width]);
    midbuf2 = &(buf[2][half_width]);
  }

  row_idx = 0;
  row_idx_plus2 = width * 2;

  for (row=0; row != QT_BAND; row+=4) {
    progress_bar(-1, -1, 80*22, (top + row), height);
    huff_ptr = NULL;
    t = getbithuff(6);
    /* Ignore those */
    getbithuff(6);
    getbithuff(6);

    cur_buf = buf;
    c = 0;

    val = val_from_last[last];
    val *= t;

    cur_buf_y = cur_buf[0];
    cur_buf_1 = cur_buf[1];

#ifndef __CC65__
    for (i=0; i != DATABUF_SIZE; i++) {
      tmp32 = val;
      if (*cur_buf_y) {
        tmp32 *= (*cur_buf_y);
        tmp32--;
        *((uint16 *)cur_buf_y) = tmp32 >> 12;
      } else {
        *((uint16 *)cur_buf_y) = 0xFFFF;
      }
      cur_buf_y++;
    }

    last = t;
#else
    __asm__("lda #<%w", DATABUF_SIZE);
    __asm__("sta %v", i);
    __asm__("lda #>%w", DATABUF_SIZE);
    __asm__("sta %v+1", i);

    setup_curbuf_y:
    __asm__("dec %v", i);
    /* load */
    __asm__("lda (%v)", cur_buf_y);
    __asm__("bne %g", not_null_buf);
    __asm__("lda (%v),y", cur_buf_y);
    __asm__("beq %g", null_buf);
    not_null_buf:

    __asm__("ldy %v+1", val);
    __asm__("sty ptr1+1");
    __asm__("ldx %v", val);
    __asm__("stx ptr1");
    /* multiply */
    __asm__("ldy #$01");
    __asm__("lda (%v),y", cur_buf_y);
    __asm__("sta ptr2+1");
    __asm__("lda (%v)", cur_buf_y);
    __asm__("sta ptr2");
    __asm__("jsr mult16x16x32_direct");
    /* decrement */
    __asm__("cmp #0");
    __asm__("bne %g", nouf12);
    __asm__("cpx #0");
    __asm__("bne %g", nouf13);
    __asm__("ldy sreg");
    __asm__("bne %g", nouf14);
    __asm__("dec sreg+1");
    nouf14:
    __asm__("dec sreg");
    nouf13:
    __asm__("dex");
    nouf12:
    // No need to DEC A, we're about to shift right

    /* Shift >> 8 */
    __asm__("txa");
    __asm__("ldx sreg");
    __asm__("ldy sreg+1");
    __asm__("sty sreg");
    __asm__("stz sreg+1");
    /* Then 4 */
    __asm__("jsr shreax4");

    __asm__("bra %g", store_buf);

    null_buf:
    __asm__("ldx #$FF");
    __asm__("tax");

    store_buf:
    __asm__("sta (%v)", cur_buf_y);
    __asm__("ldy #$01");
    __asm__("txa");
    __asm__("sta (%v),y", cur_buf_y);
    
    __asm__("clc");
    __asm__("lda %v", cur_buf_y);
    __asm__("adc #2");
    __asm__("sta %v", cur_buf_y);
    __asm__("bcc %g", noof21);
    __asm__("inc %v+1", cur_buf_y);
    noof21:
    __asm__("lda %v", i);
    __asm__("bne %g", setup_curbuf_y);
    __asm__("dec %v+1", i);
    __asm__("bpl %g", setup_curbuf_y);

    __asm__("lda %v", t);
    __asm__("sta %v", last);
#endif

#ifndef __CC65__
    for (r=0; r != 2; r++) {
      tree = (t << 7);
      *midbuf1 = tree;
      *midbuf2 = tree;
      for (tree = 1, col = half_width; col; ) {
        huff_ptr = huff[tree];
        tree = (uint8) getbithuff(8);

        if (tree) {
          col -= 2;
          if (tree == 8) {
            cur_buf_x = cur_buf_1 + col + 1;

            huff_ptr = huff_18;
            for (y=2; y; y--) {
              tmp8 = (uint8) getbithuff(8);
              *cur_buf_x = tmp8 * t;
              cur_buf_x--;
              tmp8 = (uint8) getbithuff(8);
              *cur_buf_x = tmp8 * t;
              cur_buf_x += DATABUF_SIZE + 1;
            }
          } else {
            uint16 *tmp_prevy;
            tmp16 = col + 1;
            cur_buf_prevy = cur_buf[0] + tmp16;
            cur_buf_x = cur_buf_prevy + DATABUF_SIZE;
            cur_buf_x_plus1 = cur_buf_x + 1;

            cur_huff = (uint16 *)(huff + tree + 10);

            y = 2;
            loop2:
            radc_token(tk, int8, cur_huff);
            *cur_buf_x = tk * 16;
            tmp16_2 = *cur_buf_prevy << 1;
            tmp_prevy = cur_buf_prevy++;
            tmp16_2 += *cur_buf_prevy
                       + *cur_buf_x_plus1;
            tmp16_2 >>= 2;
            *cur_buf_x += tmp16_2;

            cur_buf_x_plus1 = cur_buf_x--;
            cur_buf_prevy = tmp_prevy;

            /* Second with col - 1*/
            radc_token(tk, int8, cur_huff);
            *cur_buf_x = tk * 16;

            tmp16_2 = *cur_buf_prevy;
            cur_buf_prevy--;
            tmp16_2 += (*cur_buf_prevy << 1)
                       + *cur_buf_x_plus1;
            tmp16_2 >>= 2;
            *cur_buf_x += tmp16_2;

            y--;
            if (y == 0)
              goto loop2_done;
            cur_buf_x += DATABUF_SIZE+1;
            cur_buf_x_plus1 += DATABUF_SIZE+1;
            cur_buf_prevy += DATABUF_SIZE+1;
            goto loop2;
            loop2_done:
          }
        } else {
          do {
            if (col > 2) {
              radc_token(nreps, int8, huff_9);
              nreps++;
            } else {
              nreps = 1;
            }
            if (nreps > 8) {
              rep_loop = 8;
            } else {
              rep_loop = nreps;
            }
            rep = 0;
            do_rep_loop:
              uint16 *tmp_prevy;
              col -= 2;
              tmp16 = col + 1;
              cur_buf_prevy = cur_buf[0] + tmp16;
              cur_buf_x = cur_buf_prevy + DATABUF_SIZE;
              cur_buf_x_plus1 = cur_buf_x + 1;

              for (y=1; ; y--) {
                /* Unrolled */

                *cur_buf_x = *cur_buf_prevy << 1;
                tmp_prevy = cur_buf_prevy++;
                *cur_buf_x += *cur_buf_prevy
                           + *cur_buf_x_plus1;
                *cur_buf_x >>= 2;

                cur_buf_x_plus1 = cur_buf_x;
                cur_buf_x--;
                cur_buf_prevy = --tmp_prevy;

                if (c) {
                  *cur_buf_x = (*cur_buf_prevy 
                                + *(cur_buf_x_plus1)) << 1;
                } else {
                  *cur_buf_x = *cur_buf_prevy << 1;
                  tmp_prevy = cur_buf_prevy++;
                  *cur_buf_x += *cur_buf_prevy
                             + *cur_buf_x_plus1;
                  *cur_buf_x >>= 2;

                  cur_buf_prevy = tmp_prevy;
                }
                if (!y)
                  break;
                cur_buf_x += DATABUF_SIZE+1;
                cur_buf_x_plus1 += DATABUF_SIZE+1;
                cur_buf_prevy += DATABUF_SIZE+1;
              }
              if (rep & 1) {
                radc_token(tk, int8, huff_10);
                tmp_i16 = tk << 4;

                tmp16 = col + 1;
                cur_buf_prevy = cur_buf[0] + tmp16;
                cur_buf_x = cur_buf_prevy + DATABUF_SIZE;

                for (y=1; ; y--) {
                  /* Unrolled */
                  *cur_buf_x += tmp_i16;

                  cur_buf_x_plus1 = cur_buf_x;
                  cur_buf_x--;
                  cur_buf_prevy--;

                  *cur_buf_x += tmp_i16;
                  if (!y)
                    break;
                  cur_buf_x += DATABUF_SIZE+1;
                  cur_buf_x_plus1 += DATABUF_SIZE+1;
                  cur_buf_prevy += DATABUF_SIZE+1;
                }
              }
            rep++;
            if (rep == rep_loop)
              goto rep_loop_done;
            if (!col)
              goto rep_loop_done;
            goto do_rep_loop;

            rep_loop_done:
            y = y;
          } while (nreps == 9);
        }
      }
      raw_ptr1 = raw_image + row_idx; //FILE_IDX(row, 0);
      raw_ptr2 = raw_image + row_idx_plus2; //FILE_IDX(row + 2, 0);
      cur_buf_y = cur_buf_1;
      for (y=1; ; y++) {
        cur_buf_x = cur_buf_y;
        for (x=0; x != half_width; x++) {
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
#else

    __asm__("lda #1");
    __asm__("sta %v", r);
    r_loop:
    // for (r=0; r != 2; r++) {
      __asm__("lda %v", midbuf1);
      __asm__("sta ptr1");
      __asm__("ldx %v+1", midbuf1);
      __asm__("stx ptr1+1");
      __asm__("lda %v", midbuf2);
      __asm__("sta ptr2");
      __asm__("ldx %v+1", midbuf2);
      __asm__("stx ptr2+1");

      __asm__("ldx #0");
      __asm__("lda %v", t);
      __asm__("jsr aslax7");
      __asm__("sta %v", tree);
      __asm__("stx %v+1", tree);

      /* Update midbuf1/2 */
      __asm__("sta (ptr1)");
      __asm__("sta (ptr2)");
      __asm__("ldy #1");
      __asm__("txa");
      __asm__("sta (ptr1),y");
      __asm__("sta (ptr2),y");

      __asm__("lda #1");
      __asm__("sta %v", tree);
      __asm__("ldx #0");
      __asm__("stx %v+1", tree);

      __asm__("lda %v", half_width);
      __asm__("sta %v", col);
      __asm__("ldx %v+1", half_width);
      __asm__("stx %v+1", col);

      col_loop1:
        __asm__("lda %v", tree);
        __asm__("asl");
        __asm__("clc");
        __asm__("adc #>(%v)", huff);
        __asm__("sta %v+1", huff_ptr);
        __asm__("lda #<(%v)", huff);
        __asm__("sta %v", huff_ptr);

        __asm__("lda #8");
        __asm__("jsr %v", getbithuff);
        __asm__("sta %v", tree);
        __asm__("stx %v+1", tree);

        __asm__("bne %g", tree_not_zero);
        __asm__("cpx #0");
        __asm__("bne %g", tree_not_zero);
        __asm__("jmp %g", tree_zero);
        tree_not_zero:
          __asm__("lda %v", col);
          __asm__("sec");
          __asm__("sbc #2");
          __asm__("sta %v", col);
          __asm__("bcs %g", nouf16);
          __asm__("dec %v+1", col);
          nouf16:
          
          __asm__("lda %v", tree);
          __asm__("cmp #8");
          __asm__("bne %g", tree_not_eight);

            __asm__("lda %v", col);
            __asm__("ldx %v+1", col);
            __asm__("inc a");
            __asm__("bne %g", noof6);
            __asm__("inx");
            noof6:
            __asm__("stx tmp1");
            __asm__("asl a");
            __asm__("rol tmp1");

            __asm__("clc");
            __asm__("adc %v", cur_buf_1);
            __asm__("sta %v", cur_buf_x);
            __asm__("lda tmp1");
            __asm__("adc %v+1", cur_buf_1);
            __asm__("sta %v+1", cur_buf_x);

            __asm__("lda #2");
            __asm__("sta %v", y);

            __asm__("lda %v", huff_18);
            __asm__("sta %v", huff_ptr);
            __asm__("lda %v+1", huff_18);
            __asm__("sta %v+1", huff_ptr);

            y_loop1:
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            __asm__("ldx #0");
            __asm__("jsr pushax");
            __asm__("lda %v", t);
            __asm__("jsr tosumula0");
            __asm__("sta (%v)", cur_buf_x);
            __asm__("txa");
            __asm__("ldy #1");
            __asm__("sta (%v),y", cur_buf_x);

            __asm__("lda %v", cur_buf_x);
            __asm__("sec");
            __asm__("sbc #2");
            __asm__("sta %v", cur_buf_x);
            __asm__("bcs %g", nouf15);
            __asm__("dec %v+1", cur_buf_x);
            nouf15:

            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            __asm__("ldx #0");
            __asm__("jsr pushax");
            __asm__("lda %v", t);
            __asm__("jsr tosumula0");
            __asm__("sta (%v)", cur_buf_x);
            __asm__("txa");
            __asm__("ldy #1");
            __asm__("sta (%v),y", cur_buf_x);

            __asm__("clc");
            __asm__("lda %v", cur_buf_x);
            __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
            __asm__("sta %v", cur_buf_x);
            __asm__("lda %v+1", cur_buf_x);
            __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
            __asm__("sta %v+1", cur_buf_x);
            
            __asm__("dec %v", y);
            __asm__("bne %g", y_loop1);
            __asm__("jmp %g", tree_done);
          tree_not_eight:
            __asm__("ldx %v+1", col);
            __asm__("lda %v", col);
            __asm__("ina");
            __asm__("bne %g", noof18);
            __asm__("inx");
            noof18:
            __asm__("stx tmp1");
            /* set cur_buf_prevy */
            __asm__("asl a");
            __asm__("rol tmp1");
            __asm__("clc");
            __asm__("adc %v", cur_buf);
            __asm__("sta %v", cur_buf_prevy);
            __asm__("tax");
            __asm__("lda tmp1");
            __asm__("adc %v+1", cur_buf);
            __asm__("sta %v+1", cur_buf_prevy);
            __asm__("tay");
            /* set cur_buf_x (prevy in YX) */
            __asm__("clc");
            __asm__("txa");
            __asm__("adc #<%w", DATABUF_SIZE * 2);
            __asm__("sta %v", cur_buf_x);
            __asm__("tax");
            __asm__("tya");
            __asm__("adc #>%w", DATABUF_SIZE * 2);
            __asm__("sta %v+1", cur_buf_x);
            __asm__("tay");
            /* set cur_buf_x_plus1 (inc2 because uint16)*/
            __asm__("inx");
            __asm__("bne %g", noof19);
            __asm__("iny");
            noof19:
            __asm__("inx");
            __asm__("bne %g", noof20);
            __asm__("iny");
            noof20:
            __asm__("stx %v", cur_buf_x_plus1);
            __asm__("sty %v+1", cur_buf_x_plus1);

            //cur_huff = huff[tree + 10];
            __asm__("lda %v", tree);
            __asm__("clc");
            __asm__("adc #10");
            __asm__("asl a");
            __asm__("clc");
            __asm__("adc #>(%v)", huff); /* adding to high byte because bidimensional */
            __asm__("sta %v+1", cur_huff);
            __asm__("lda #<(%v)", huff);
            __asm__("sta %v", cur_huff);

            __asm__("lda #2");
            __asm__("sta %v", y);
            loop2:
            __asm__("lda %v", cur_huff);
            __asm__("sta %v", huff_ptr);
            __asm__("lda %v+1", cur_huff);
            __asm__("sta %v+1", huff_ptr);
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            __asm__("sta %v", tk);

            __asm__("ldx #0");
            __asm__("cmp #0");
            __asm__("bpl %g", pos1);
            __asm__("dex");
            pos1:
            __asm__("jsr aslax4");
            
            __asm__("sta (%v)", cur_buf_x);
            __asm__("txa");
            __asm__("ldy #1");
            __asm__("sta (%v),y", cur_buf_x);

            __asm__("lda (%v),y", cur_buf_prevy);
            __asm__("sta tmp1");
            __asm__("lda (%v)", cur_buf_prevy);
            __asm__("asl a");
            __asm__("rol tmp1");
            __asm__("ldx tmp1");
            __asm__("sta %v", tmp16_2);
            __asm__("stx %v+1", tmp16_2);

            __asm__("lda %v", cur_buf_prevy);
            __asm__("pha");
            __asm__("ldx %v+1", cur_buf_prevy);
            __asm__("phx");

            __asm__("clc");
            __asm__("adc #2");
            __asm__("sta %v", cur_buf_prevy);
            __asm__("bcc %g", noof2);
            __asm__("inc %v+1", cur_buf_prevy);
            noof2:

            __asm__("clc");
            __asm__("lda (%v)", cur_buf_prevy);
            __asm__("adc (%v)", cur_buf_x_plus1);
            __asm__("tax");
            __asm__("ldy #$01");
            __asm__("lda (%v),y", cur_buf_prevy);
            __asm__("adc (%v),y", cur_buf_x_plus1);
            __asm__("tay");

            /* add tmp16_2 */
            __asm__("txa");
            __asm__("clc");
            __asm__("adc %v", tmp16_2);
            __asm__("tax");
            __asm__("tya");
            __asm__("adc %v+1", tmp16_2);
            /* shift right 2 */
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("lsr tmp1");
            __asm__("ror a");
            __asm__("lsr tmp1");
            __asm__("ror a");

            __asm__("clc");
            __asm__("adc (%v)", cur_buf_x);
            __asm__("sta (%v)", cur_buf_x);

            __asm__("lda tmp1");
            __asm__("ldy #$01");
            __asm__("adc (%v),y", cur_buf_x);
            __asm__("sta (%v),y", cur_buf_x);

            __asm__("ldx %v+1", cur_buf_x);
            __asm__("stx %v+1", cur_buf_x_plus1);
            __asm__("lda %v", cur_buf_x);
            __asm__("sta %v", cur_buf_x_plus1);
            __asm__("sec");
            __asm__("sbc #2");
            __asm__("bcs %g", nouf6);
            __asm__("dec %v+1", cur_buf_x);
            nouf6:
            __asm__("sta %v", cur_buf_x);

            __asm__("plx");
            __asm__("stx %v+1", cur_buf_prevy);
            __asm__("pla");
            __asm__("sta %v", cur_buf_prevy);

            /* Second with col - 1*/
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            __asm__("sta %v", tk);

            __asm__("ldx #0");
            __asm__("cmp #0");
            __asm__("bpl %g", pos2);
            __asm__("dex");
            pos2:
            __asm__("jsr aslax4");
            
            __asm__("sta (%v)", cur_buf_x);
            __asm__("txa");
            __asm__("ldy #1");
            __asm__("sta (%v),y", cur_buf_x);


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
            __asm__("sta tmp1");
            __asm__("pla"); /* cur_buf_prevy<<1 + tmp16_2 + cur_buf_xplus1 in AX */

            /* shift right 2 */
            __asm__("lsr tmp1");
            __asm__("ror a");
            __asm__("lsr tmp1");
            __asm__("ror a");

            /* Store to cur_buf_x */
            __asm__("clc");
            __asm__("adc (%v)", cur_buf_x);
            __asm__("sta (%v)", cur_buf_x);

            __asm__("lda tmp1");
            __asm__("adc (%v),y", cur_buf_x);
            __asm__("sta (%v),y", cur_buf_x);

            __asm__("dec %v", y);
            __asm__("beq %g", loop2_done);

            /* shift pointers */
            __asm__("clc");
            __asm__("lda %v", cur_buf_x);
            __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
            __asm__("sta %v", cur_buf_x);
            __asm__("lda %v+1", cur_buf_x);
            __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
            __asm__("sta %v+1", cur_buf_x);

            __asm__("lda %v", cur_buf_x_plus1);
            __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
            __asm__("sta %v", cur_buf_x_plus1);
            __asm__("lda %v+1", cur_buf_x_plus1);
            __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
            __asm__("sta %v+1", cur_buf_x_plus1);

            __asm__("lda %v", cur_buf_prevy);
            __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
            __asm__("sta %v", cur_buf_prevy);
            __asm__("lda %v+1", cur_buf_prevy);
            __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
            __asm__("sta %v+1", cur_buf_prevy);

            __asm__("jmp %g", loop2);
            loop2_done:
          __asm__("jmp %g", tree_done);
        tree_zero:
          nine_reps_loop:
            __asm__("lda %v", col);
            __asm__("cmp #3");
            __asm__("bcs %g", col_gt2);
            __asm__("ldx %v+1", col);
            __asm__("bne %g", col_gt2);
            __asm__("lda #1"); /* nreps */
            __asm__("bra %g", check_nreps);
            col_gt2:
            __asm__("lda %v", huff_9);
            __asm__("sta %v", huff_ptr);
            __asm__("lda %v+1", huff_9);
            __asm__("sta %v+1", huff_ptr);
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            __asm__("inc a");
            check_nreps:
            __asm__("sta %v", nreps);

            __asm__("cmp #9");
            __asm__("bcc %g", nreps_check_done);
            __asm__("lda #8");
            nreps_check_done:
            __asm__("sta %v", rep_loop);

            __asm__("stz %v", rep);
            do_rep_loop:
              __asm__("ldx %v+1", col);
              __asm__("lda %v", col);
              __asm__("bne %g", nouf3);
              __asm__("dex");
              nouf3:
              __asm__("dec a");
              __asm__("stx tmp1");
              __asm__("pha");
              __asm__("bne %g", nouf4);
              __asm__("dex");
              nouf4:
              __asm__("dea");
              __asm__("stx %v+1", col);
              __asm__("sta %v", col);
              __asm__("pla");
              /* set cur_buf_prevy */
              __asm__("asl a");
              __asm__("rol tmp1");
              __asm__("clc");
              __asm__("adc %v", cur_buf);
              __asm__("sta %v", cur_buf_prevy);
              __asm__("tax");
              __asm__("lda tmp1");
              __asm__("adc %v+1", cur_buf);
              __asm__("sta %v+1", cur_buf_prevy);
              __asm__("tay");
              /* set cur_buf_x (prevy in YX) */
              __asm__("clc");
              __asm__("txa");
              __asm__("adc #<%w", DATABUF_SIZE * 2);
              __asm__("sta %v", cur_buf_x);
              __asm__("tax");
              __asm__("tya");
              __asm__("adc #>%w", DATABUF_SIZE * 2);
              __asm__("sta %v+1", cur_buf_x);
              __asm__("tay");
              /* set cur_buf_x_plus1 (inc2 because uint16)*/
              __asm__("inx");
              __asm__("bne %g", noof15);
              __asm__("iny");
              noof15:
              __asm__("inx");
              __asm__("bne %g", noof16);
              __asm__("iny");
              noof16:
              __asm__("stx %v", cur_buf_x_plus1);
              __asm__("sty %v+1", cur_buf_x_plus1);

              __asm__("lda #2");
              __asm__("sta %v", y);
              loop3:
              __asm__("ldy #$01");
              __asm__("lda (%v),y", cur_buf_prevy);
              __asm__("sta tmp1");
              __asm__("lda (%v)", cur_buf_prevy);
              __asm__("asl a");
              __asm__("rol tmp1");
              __asm__("sta (%v)", cur_buf_x);
              __asm__("lda tmp1");
              __asm__("sta (%v),y", cur_buf_x);

              __asm__("lda %v", cur_buf_prevy);
              __asm__("pha");
              __asm__("ldx %v+1", cur_buf_prevy);
              __asm__("phx");

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
              __asm__("tax");
              __asm__("tya");
              __asm__("ldy #$01");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta tmp1");
              __asm__("txa");

              /* shift right 2 */
              __asm__("lsr tmp1");
              __asm__("ror a");
              __asm__("lsr tmp1");
              __asm__("ror a");

              __asm__("sta (%v)", cur_buf_x);
              __asm__("lda tmp1");
              __asm__("sta (%v),y", cur_buf_x);

              __asm__("lda %v", cur_buf_x);
              __asm__("sta %v", cur_buf_x_plus1);
              __asm__("ldx %v+1", cur_buf_x);
              __asm__("stx %v+1", cur_buf_x_plus1);
              __asm__("sec");
              __asm__("sbc #2");
              __asm__("sta %v", cur_buf_x);
              __asm__("bcs %g", nouf8);
              __asm__("dex");
              nouf8:
              __asm__("stx %v+1", cur_buf_x);
              
              __asm__("plx");
              __asm__("pla");
              __asm__("sec");
              __asm__("sbc #2");
              __asm__("sta %v", cur_buf_prevy);
              __asm__("bcs %g", nouf7);
              __asm__("dex");
              nouf7:
              __asm__("stx %v+1", cur_buf_prevy);

              /* Second */
              __asm__("lda %v", c);
              __asm__("ora %v+1", c);
              __asm__("beq %g", c_zero);

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
                __asm__("bra %g", c_done);
              c_zero:
                __asm__("ldy #$01");
                __asm__("lda (%v),y", cur_buf_prevy);
                __asm__("sta tmp1");
                __asm__("lda (%v)", cur_buf_prevy);
                __asm__("asl a");
                __asm__("rol tmp1");
                __asm__("sta (%v)", cur_buf_x);
                __asm__("lda tmp1");
                __asm__("sta (%v),y", cur_buf_x);

                __asm__("lda %v", cur_buf_prevy);
                __asm__("pha");
                __asm__("ldx %v+1", cur_buf_prevy);
                __asm__("phx");


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
                __asm__("tax");
                __asm__("tya");
                __asm__("ldy #$01");
                __asm__("adc (%v),y", cur_buf_x);
                __asm__("sta tmp1");
                __asm__("txa");

                /* shift right 2 */
                __asm__("lsr tmp1");
                __asm__("ror a");
                __asm__("lsr tmp1");
                __asm__("ror a");

                __asm__("sta (%v)", cur_buf_x);
                __asm__("lda tmp1");
                __asm__("sta (%v),y", cur_buf_x);

                __asm__("plx");
                __asm__("stx %v+1", cur_buf_prevy);
                __asm__("pla");
                __asm__("sta %v", cur_buf_prevy);
                c_done:
                __asm__("dec %v", y);
                __asm__("beq %g", loop3_done);

              /* shift pointers */
              __asm__("clc");
              __asm__("lda %v", cur_buf_x);
              __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
              __asm__("sta %v", cur_buf_x);
              __asm__("lda %v+1", cur_buf_x);
              __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
              __asm__("sta %v+1", cur_buf_x);

              __asm__("lda %v", cur_buf_x_plus1);
              __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
              __asm__("sta %v", cur_buf_x_plus1);
              __asm__("lda %v+1", cur_buf_x_plus1);
              __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
              __asm__("sta %v+1", cur_buf_x_plus1);

              __asm__("lda %v", cur_buf_prevy);
              __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
              __asm__("sta %v", cur_buf_prevy);
              __asm__("lda %v+1", cur_buf_prevy);
              __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
              __asm__("sta %v+1", cur_buf_prevy);

              __asm__("jmp %g", loop3);
              loop3_done:

              __asm__("lda %v", rep);
              __asm__("and #1");
              __asm__("beq %g", rep_even);
              __asm__("lda %v", huff_10);
              __asm__("sta %v", huff_ptr);
              __asm__("lda %v+1", huff_10);
              __asm__("sta %v+1", huff_ptr);
              __asm__("lda #8");
              __asm__("jsr %v", getbithuff);
              __asm__("sta %v", tk);

              __asm__("ldx #0");
              __asm__("cmp #0");
              __asm__("bpl %g", pos3);
              __asm__("dex");
              pos3:
              __asm__("jsr aslax4");
              __asm__("sta %v", tmp_i16);
              __asm__("stx %v+1", tmp_i16);

              __asm__("ldx %v+1", col);
              __asm__("lda %v", col);
              __asm__("ina");
              __asm__("bne %g", noof17);
              __asm__("inx");
              noof17:
              __asm__("stx tmp1");
              /* set cur_buf_prevy */
              __asm__("asl a");
              __asm__("rol tmp1");
              __asm__("clc");
              __asm__("adc %v", cur_buf);
              __asm__("sta %v", cur_buf_prevy);
              __asm__("tax");
              __asm__("lda tmp1");
              __asm__("adc %v+1", cur_buf);
              __asm__("sta %v+1", cur_buf_prevy);
              __asm__("tay");
              /* set cur_buf_x (prevy in YX) */
              __asm__("clc");
              __asm__("txa");
              __asm__("adc #<%w", DATABUF_SIZE * 2);
              __asm__("sta %v", cur_buf_x);
              __asm__("tya");
              __asm__("adc #>%w", DATABUF_SIZE * 2);
              __asm__("sta %v+1", cur_buf_x);

                __asm__("lda #2");
                __asm__("sta %v", y);
                loop4:
                  __asm__("ldy #$01");
                  __asm__("clc");
                  __asm__("lda %v", tmp_i16);
                  __asm__("adc (%v)", cur_buf_x);
                  __asm__("sta (%v)", cur_buf_x);
                  __asm__("lda %v+1", tmp_i16);
                  __asm__("adc (%v),y", cur_buf_x);
                  __asm__("sta (%v),y", cur_buf_x);

                  __asm__("lda %v", cur_buf_x);
                  __asm__("sta %v", cur_buf_x_plus1);
                  __asm__("ldx %v+1", cur_buf_x);
                  __asm__("stx %v+1", cur_buf_x_plus1);

                  __asm__("sec");
                  __asm__("sbc #2");
                  __asm__("sta %v", cur_buf_x);
                  __asm__("bcs %g", nouf9);
                  __asm__("dex");
                  nouf9:
                  __asm__("stx %v+1", cur_buf_x);

                  __asm__("lda %v", cur_buf_prevy);
                  __asm__("sec");
                  __asm__("sbc #2");
                  __asm__("sta %v", cur_buf_prevy);
                  __asm__("bcs %g", nouf10);
                  __asm__("dec %v+1", cur_buf_prevy);
                  nouf10:

                  __asm__("ldy #$01");
                  __asm__("clc");
                  __asm__("lda %v", tmp_i16);
                  __asm__("adc (%v)", cur_buf_x);
                  __asm__("sta (%v)", cur_buf_x);
                  __asm__("lda %v+1", tmp_i16);
                  __asm__("adc (%v),y", cur_buf_x);
                  __asm__("sta (%v),y", cur_buf_x);
                  
                  __asm__("dec %v", y);
                  __asm__("beq %g", loop4_done);

                  /* shift pointers */
                  __asm__("clc");
                  __asm__("lda %v", cur_buf_x);
                  __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
                  __asm__("sta %v", cur_buf_x);
                  __asm__("lda %v+1", cur_buf_x);
                  __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
                  __asm__("sta %v+1", cur_buf_x);

                  __asm__("lda %v", cur_buf_x_plus1);
                  __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
                  __asm__("sta %v", cur_buf_x_plus1);
                  __asm__("lda %v+1", cur_buf_x_plus1);
                  __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
                  __asm__("sta %v+1", cur_buf_x_plus1);

                  __asm__("lda %v", cur_buf_prevy);
                  __asm__("adc #<%w", 2*(DATABUF_SIZE+1));
                  __asm__("sta %v", cur_buf_prevy);
                  __asm__("lda %v+1", cur_buf_prevy);
                  __asm__("adc #>%w", 2*(DATABUF_SIZE+1));
                  __asm__("sta %v+1", cur_buf_prevy);
                  __asm__("jmp %g", loop4);
                loop4_done:
              rep_even:
            __asm__("inc %v", rep);
            __asm__("lda %v", rep);
            __asm__("cmp %v", rep_loop);
            __asm__("beq %g", rep_loop_done);
            __asm__("lda %v", col);
            __asm__("ora %v+1", col);
            __asm__("beq %g", rep_loop_done);
            __asm__("jmp %g", do_rep_loop);
            rep_loop_done:
            __asm__("lda %v", nreps);
            __asm__("cmp #9");
            __asm__("bne %g", nine_reps_loop_done);
            __asm__("jmp %g", nine_reps_loop);
          nine_reps_loop_done:
        tree_done:
      __asm__("lda %v", col);
      __asm__("ora %v+1", col);
      __asm__("beq %g", col_loop1_done);
      __asm__("jmp %g", col_loop1);
      col_loop1_done:

      __asm__("clc");
      __asm__("lda #<(%v)", raw_image);
      __asm__("adc %v", row_idx);
      __asm__("sta %v", raw_ptr1);
      __asm__("lda #>(%v)", raw_image);
      __asm__("adc %v+1", row_idx);
      __asm__("sta %v+1", raw_ptr1);

      __asm__("lda #<(%v)", raw_image);
      __asm__("adc %v", row_idx_plus2);
      __asm__("sta %v", raw_ptr2);
      __asm__("lda #>(%v)", raw_image);
      __asm__("adc %v+1", row_idx_plus2);
      __asm__("sta %v+1", raw_ptr2);

      __asm__("lda #2");
      __asm__("sta %v", y);

      __asm__("ldy %v", cur_buf_1);
      __asm__("sty %v", cur_buf_y);
      __asm__("lda %v+1", cur_buf_1);
      __asm__("sta %v+1", cur_buf_y);

      loop5:
        __asm__("sty %v", cur_buf_x);
        __asm__("sta %v+1", cur_buf_x);

        __asm__("lda %v", half_width);
        __asm__("sta %v", x);
        __asm__("lda %v+1", half_width);
        __asm__("sta %v+1", x);

        x_loop:
          __asm__("ldy #1");
          __asm__("lda (%v),y", cur_buf_x);
          __asm__("sta ptr2+1");
          __asm__("lda (%v)", cur_buf_x);
          __asm__("sta ptr2");
          __asm__("tax");
          __asm__("ldy %v", t);
          __asm__("jsr approx_div16x8_direct");
          __asm__("cpx #0");
          __asm__("beq %g", no_val_clamp);
          __asm__("lda #$FF");
          no_val_clamp:

          __asm__("ldy %v", r);
          __asm__("bne %g", r_zero);
          __asm__("sta (%v)", raw_ptr2);
          __asm__("bra %g", r_done);
          r_zero:
          __asm__("sta (%v)", raw_ptr1);
          r_done:

          __asm__("clc");
          __asm__("lda %v", cur_buf_x);
          __asm__("adc #2");
          __asm__("sta %v", cur_buf_x);
          __asm__("bcc %g", noof24);
          __asm__("inc %v+1", cur_buf_x);
          __asm__("clc");
          noof24:

          __asm__("lda %v", raw_ptr1);
          __asm__("adc #2");
          __asm__("sta %v", raw_ptr1);
          __asm__("bcc %g", noof25);
          __asm__("inc %v+1", raw_ptr1);
          __asm__("clc");
          noof25:

          __asm__("lda %v", raw_ptr2);
          __asm__("adc #2");
          __asm__("sta %v", raw_ptr2);
          __asm__("bcc %g", noof26);
          __asm__("inc %v+1", raw_ptr2);
          __asm__("clc");
          noof26:

        __asm__("lda %v", x);
        __asm__("bne %g", nouf17);
        __asm__("dec %v", x);
        __asm__("dec %v+1", x);
        __asm__("bmi %g", x_loop_done);
        nouf17:
        __asm__("dec %v", x);
        __asm__("jmp %g", x_loop);

        x_loop_done:
        __asm__("dec %v", y);
        __asm__("beq %g", loop5_done);
        
        __asm__("inc %v", raw_ptr1);
        __asm__("bne %g", noof22);
        __asm__("inc %v+1", raw_ptr1);
        noof22:

        __asm__("inc %v", raw_ptr2);
        __asm__("bne %g", noof23);
        __asm__("inc %v+1", raw_ptr2);
        noof23:

        __asm__("clc");
        __asm__("lda %v", cur_buf_y);
        __asm__("adc #<%w", 2*DATABUF_SIZE);
        __asm__("sta %v", cur_buf_y);
        __asm__("tay");
        __asm__("lda %v+1", cur_buf_y);
        __asm__("adc #>%w", 2*DATABUF_SIZE);
        __asm__("sta %v+1", cur_buf_y);

        __asm__("jmp %g", loop5);

      loop5_done:
      __asm__("clc");
      __asm__("ldx %v+1", cur_buf); /* cur_buf[0]+1 */
      __asm__("lda %v", cur_buf);
      __asm__("adc #2");
      __asm__("bcc %g", noof27);
      __asm__("inx");
      __asm__("clc");
      noof27:
      __asm__("jsr pushax");

      __asm__("clc");
      __asm__("lda %v", cur_buf); /* curbuf[2] */
      __asm__("adc #<%w", DATABUF_SIZE*4);
      __asm__("tay");
      __asm__("lda %v+1", cur_buf);
      __asm__("adc #>%w", DATABUF_SIZE*4);
      __asm__("tax");
      __asm__("tya");
      __asm__("jsr pushax");
      
      __asm__("lda #<%w", 2*(DATABUF_SIZE-1));
      __asm__("ldx #>%w", 2*(DATABUF_SIZE-1));
      __asm__("jsr _memcpy");
    // }
    __asm__("dec %v", r);
    __asm__("bmi %g", r_loop_done);
    __asm__("jmp %g", r_loop);
    r_loop_done:
    __asm__("lda %v", cur_buf);
    __asm__("clc");
    __asm__("adc #%b", (3*3));
    __asm__("sta %v", cur_buf);
    __asm__("bcc %g", noof28);
    __asm__("inc %v+1", cur_buf);
    noof28:
#endif

#ifndef __CC65__
    /* Consume RADC tokens but don't care about them. */
    for (c=1; c != 3; c++) {
      for (tree = 1, col = half_width; col; ) {
        cur_huff = huff[tree];
        radc_token(tree, uint8, cur_huff);
        if (tree) {
          col -= 2;
          cur_huff = huff[tree + 10];
          skip_radc_token(cur_huff);
          skip_radc_token(cur_huff);
          skip_radc_token(cur_huff);
          skip_radc_token(cur_huff);
        } else {
          do {
            if (col > 2) {
              radc_token(nreps, int8, huff_9);
              nreps++;
            } else {
              nreps = 1;
            }
            if (nreps > 8) {
              rep_loop = 8;
            } else {
              rep_loop = nreps;
            }
            for (rep=0; rep != rep_loop && col; rep++) {
              col -= 2;
              if (rep & 1) {
                skip_radc_token(huff_10);
              }
            }
          } while (nreps == 9);
        }
      }
    }
#else
    /* Consume RADC tokens but don't care about them. */
    __asm__("lda #2");
    __asm__("sta %v", c);
    c_loop:
      __asm__("lda #1");
      __asm__("sta %v", tree);
      __asm__("stz %v+1", tree);
      __asm__("lda %v", half_width);
      __asm__("sta %v", col);
      __asm__("lda %v+1", half_width);
      __asm__("sta %v+1", col);
      
      col_loop2:
        __asm__("lda %v", tree);
        __asm__("asl a");
        __asm__("clc");
        __asm__("adc #>(%v)", huff);
        __asm__("sta %v+1", huff_ptr);
        __asm__("lda #<(%v)", huff);
        __asm__("sta %v", huff_ptr);

        __asm__("lda #8");
        __asm__("jsr %v", getbithuff);
        __asm__("sta %v", tree);
        __asm__("stx %v+1", tree);

        __asm__("bne %g", tree_not_zero_2);
        __asm__("cpx #0");
        __asm__("beq %g", tree_zero_2);
        tree_not_zero_2:
          __asm__("lda %v", col);
          __asm__("sec");
          __asm__("sbc #2");
          __asm__("sta %v", col);
          __asm__("bcs %g", nouf20);
          __asm__("dec %v+1", col);
          nouf20:

          //cur_huff = huff[tree + 10];
          __asm__("lda %v", tree);
          __asm__("clc");
          __asm__("adc #10");
          __asm__("asl a");
          __asm__("adc #>(%v)", huff);
          __asm__("sta %v+1", huff_ptr);
          __asm__("lda #<(%v)", huff);
          __asm__("sta %v", huff_ptr);

          __asm__("lda #8");
          __asm__("jsr %v", getbithuff);
          __asm__("lda #8");
          __asm__("jsr %v", getbithuff);
          __asm__("lda #8");
          __asm__("jsr %v", getbithuff);
          __asm__("lda #8");
          __asm__("jsr %v", getbithuff);

        __asm__("jmp %g", tree_zero_2_done);
        tree_zero_2:
            __asm__("lda %v", col);
            __asm__("cmp #3");
            __asm__("bcs %g", col_gt2_2);
            __asm__("ldx %v+1", col);
            __asm__("bne %g", col_gt2_2);

            __asm__("lda #1"); /* nreps */
            __asm__("bra %g", check_nreps_2);

            col_gt2_2:
            __asm__("lda %v", huff_9);
            __asm__("sta %v", huff_ptr);
            __asm__("lda %v+1", huff_9);
            __asm__("sta %v+1", huff_ptr);
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            __asm__("inc a");

            check_nreps_2:
            __asm__("sta %v", nreps);

            __asm__("cmp #9");
            __asm__("bcc %g", nreps_check_done_2);
            __asm__("lda #8");
            nreps_check_done_2:
            __asm__("sta %v", rep_loop);

            __asm__("lda %v", huff_10);
            __asm__("sta %v", huff_ptr);
            __asm__("lda %v+1", huff_10);
            __asm__("sta %v+1", huff_ptr);

            __asm__("stz %v", rep);
            do_rep_loop_2:
            __asm__("lda %v", col);
            __asm__("sec");
            __asm__("sbc #2");
            __asm__("sta %v", col);
            __asm__("bcs %g", nouf21);
            __asm__("dec %v+1", col);
            nouf21:

            __asm__("lda %v", rep);
            __asm__("and #1");
            __asm__("beq %g", rep_even_2);
            
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);

            rep_even_2:
            __asm__("inc %v", rep);
            __asm__("lda %v", rep);
            __asm__("cmp %v", rep_loop);
            __asm__("beq %g", rep_loop_2_done);
            __asm__("lda %v", col);
            __asm__("bne %g", do_rep_loop_2);
            __asm__("lda %v+1", col);
            __asm__("bne %g", do_rep_loop_2);

            rep_loop_2_done:
          __asm__("lda %v", nreps);
          __asm__("cmp #9");
          __asm__("beq %g", tree_zero_2);
        tree_zero_2_done:
      __asm__("lda %v", col);
      __asm__("bne %g", col_loop2);
      __asm__("lda %v+1", col);
      __asm__("bne %g", col_loop2);

    __asm__("dec %v", c);
    __asm__("beq %g", c_loop_done);
    __asm__("jmp %g", c_loop);
    c_loop_done:
#endif

#ifndef __CC65__
    raw_ptr1 = raw_image + row_idx - 1;
    for (y = 4; y; y--) {
      tmp8 = y & 1;
      if (!tmp8) {
        raw_ptr1++;
        x = 0;
        stop = width;
      } else {
        x = 1;
        stop = width - 1;
      }
      copy_again:
      *(raw_ptr1 + 1) = *raw_ptr1;
      raw_ptr1++;
      raw_ptr1++;
      x+=2;
      if (x != stop)
        goto copy_again;
      if (!tmp8) {
        raw_ptr1--;
      }
    }

    row_idx += row_idx_shift;
    row_idx_plus2 += row_idx_shift;

#else
    //raw_ptr1 = raw_image + row_idx - 1;
    __asm__("clc");
    __asm__("lda %v", row_idx);
    __asm__("adc #<(%v)", raw_image);
    __asm__("tay");
    __asm__("lda %v+1", row_idx);
    __asm__("adc #>(%v)", raw_image);
    __asm__("tax");
    __asm__("tya");
    __asm__("bne %g", nouf19);
    __asm__("dex");
    nouf19:
    __asm__("dec a");
    __asm__("sta %v", raw_ptr1);
    __asm__("stx %v+1", raw_ptr1);

    __asm__("ldy #4");
    __asm__("sty %v", y);
    y_loop:
    __asm__("lda %v", width);
    __asm__("sta %v", stop);
    __asm__("lda %v+1", width);
    __asm__("sta %v+1", stop);

    __asm__("tya");
    __asm__("and #1");
    __asm__("bne %g", even_y);

    /* inc ptr */
    __asm__("inc %v", raw_ptr1);
    __asm__("bne %g", noof14);
    __asm__("inc %v+1", raw_ptr1);
    noof14:
    __asm__("lda #0");
    __asm__("bra %g", set_x);

    even_y:
    /* decrement stop for != comparison */
    __asm__("lda %v", stop);
    __asm__("bne %g", nouf18);
    __asm__("dec %v+1", stop);
    nouf18:
    __asm__("dec %v", stop);
    __asm__("lda #1");

    set_x:
    __asm__("sta %v", x);
    __asm__("stz %v+1", x);
    

    copy_again:
    __asm__("lda (%v)", raw_ptr1);
    __asm__("inc %v", raw_ptr1);
    __asm__("bne %g", noof10);
    __asm__("inc %v+1", raw_ptr1);
    noof10:
    __asm__("sta (%v)", raw_ptr1);
    __asm__("inc %v", raw_ptr1);
    __asm__("bne %g", noof11);
    __asm__("inc %v+1", raw_ptr1);
    noof11:

    __asm__("clc");
    __asm__("lda %v", x);
    __asm__("adc #2");
    __asm__("sta %v", x);
    __asm__("bcc %g",noof12);
    __asm__("inc %v+1", x);
    noof12:

    __asm__("cmp %v", stop);
    __asm__("bne %g", copy_again);
    __asm__("lda %v+1", x);
    __asm__("cmp %v+1", stop);
    __asm__("bne %g", copy_again);

    __asm__("tya");
    __asm__("and #1");
    __asm__("bne %g", even_y2);

    __asm__("lda %v", raw_ptr1);
    __asm__("bne %g", nouf2);
    __asm__("dec %v+1", raw_ptr1);
    nouf2:
    __asm__("dec %v", raw_ptr1);
    even_y2:
    /* Finish Y loop */
    __asm__("dey");
    __asm__("bne %g", y_loop);

    __asm__("clc");
    __asm__("lda %v", row_idx);
    __asm__("adc %v", row_idx_shift);
    __asm__("sta %v", row_idx);
    __asm__("lda %v+1", row_idx);
    __asm__("adc %v+1", row_idx_shift);
    __asm__("sta %v+1", row_idx);

    __asm__("lda %v", row_idx_plus2);
    __asm__("adc %v", row_idx_shift);
    __asm__("sta %v", row_idx_plus2);
    __asm__("lda %v+1", row_idx_plus2);
    __asm__("adc %v+1", row_idx_shift);
    __asm__("sta %v+1", row_idx_plus2);
#endif
  }
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
