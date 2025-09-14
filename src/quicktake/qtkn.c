/*
   QTKN (QuickTake 150) decoding algorithm
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
#include "progress_bar.h"
#include "qt-conv.h"
#include "qtk_bithuff.h"
#include "approxdiv16x8.h"


/* Shared with qt-conv.c */
char magic[5] = QTKN_MAGIC;
char *model = "150";

extern uint8 cache[CACHE_SIZE];
uint8 *cache_start = cache;
uint8 raw_image[RAW_IMAGE_SIZE];

#define WIDTH 640U
#define HEIGHT 480U

static uint16 val_from_last[256] = {
  0x0000, 0x1000, 0x0800, 0x0555, 0x0400, 0x0333, 0x02ab, 0x0249, 0x0200, 0x01c7, 0x019a, 0x0174, 0x0155, 0x013b, 0x0125, 0x0111, 0x0100,
  0x00f1, 0x00e4, 0x00d8, 0x00cd, 0x00c3, 0x00ba, 0x00b2, 0x00ab, 0x00a4, 0x009e, 0x0098, 0x0092, 0x008d, 0x0089, 0x0084, 0x0080,
  0x007c, 0x0078, 0x0075, 0x0072, 0x006f, 0x006c, 0x0069, 0x0066, 0x0064, 0x0062, 0x005f, 0x005d, 0x005b, 0x0059, 0x0057, 0x0055,
  0x0054, 0x0052, 0x0050, 0x004f, 0x004d, 0x004c, 0x004a, 0x0049, 0x0048, 0x0047, 0x0045, 0x0044, 0x0043, 0x0042, 0x0041, 0x0040,
  0x003f, 0x003e, 0x003d, 0x003c, 0x003b, 0x003b, 0x003a, 0x0039, 0x0038, 0x0037, 0x0037, 0x0036, 0x0035, 0x0035, 0x0034, 0x0033,
  0x0033, 0x0032, 0x0031, 0x0031, 0x0030, 0x0030, 0x002f, 0x002f, 0x002e, 0x002e, 0x002d, 0x002d, 0x002c, 0x002c, 0x002b, 0x002b,
  0x002a, 0x002a, 0x0029, 0x0029, 0x0029, 0x0028, 0x0028, 0x0027, 0x0027, 0x0027, 0x0026, 0x0026, 0x0026, 0x0025, 0x0025, 0x0025,
  0x0024, 0x0024, 0x0024, 0x0023, 0x0023, 0x0023, 0x0022, 0x0022, 0x0022, 0x0022, 0x0021, 0x0021, 0x0021, 0x0021, 0x0020, 0x0020,
  0x0020, 0x0020, 0x001f, 0x001f, 0x001f, 0x001f, 0x001e, 0x001e, 0x001e, 0x001e, 0x001d, 0x001d, 0x001d, 0x001d, 0x001d, 0x001c,
  0x001c, 0x001c, 0x001c, 0x001c, 0x001b, 0x001b, 0x001b, 0x001b, 0x001b, 0x001b, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a, 0x001a,
  0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0018, 0x0017, 0x0017,
  0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0017, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0016, 0x0015, 0x0015,
  0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0015, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014,
  0x0014, 0x0014, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0013, 0x0012, 0x0012, 0x0012,
  0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0012, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011,
  0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0011, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010
};

/* note that each huff[x] share the same low byte addr */
static int16 x, s, i;
static uint16 c, col;
static uint8 r, nreps, rep, row, y, t, rep_loop, tree;

/* Wastes bytes, but simplifies adds */

static uint16 val;
static int8 tk;
#ifndef __CC65__
static uint8 tmp8;
static uint16 tmp16;
static uint32 tmp32;
#endif

#ifdef __CC65__
#define USEFUL_DATABUF_SIZE 321
#define DATABUF_SIZE 0x300 /* align things */
extern uint16 buf_0[DATABUF_SIZE/2];
extern uint16 buf_1[DATABUF_SIZE/2];
extern uint16 buf_2[DATABUF_SIZE/2];

extern uint8 huff_split[19*2][256];
extern uint8 huff_num;

#define raw_ptr1 zp6ip

#define cur_buf_x zp2ip
#define cur_buf_prev zp12ip

#define cur_buf_xh zp8ip
#define cur_buf_prevh zp10ip
#else
#define USEFUL_DATABUF_SIZE 321
#define DATABUF_SIZE 512 /* align things */
static uint16 buf_0[DATABUF_SIZE];
static uint16 buf_1[DATABUF_SIZE];
static uint16 buf_2[DATABUF_SIZE];

uint8 huff_split[19*2][256];
uint8 huff_num;

static uint8 *raw_ptr1;
static uint16 *cur_buf_prev;
static uint16 *cur_buf_x;
#endif
static uint8 *row_idx, *row_idx_plus2;

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

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

static void init_row(void) {
#ifndef __CC65__
    for (i=0; i != USEFUL_DATABUF_SIZE; i++) {
      tmp32 = val;
      if (*cur_buf_x) {
        tmp32 *= (*cur_buf_x);
        tmp32--;
        *((uint16 *)cur_buf_x) = tmp32 >> 12;
      } else {
        // tmp32 *= 0  == 0
        // tmp32--     == 0xFFFF
        // tmp32 >> 12 == 0x0F
        *((uint16 *)cur_buf_x) = 0x0F;
      }
      cur_buf_x++;
    }

    last = t;
#else
    __asm__("lda #<%w", USEFUL_DATABUF_SIZE);
    __asm__("sta %v", i);
    __asm__("lda #>%w", USEFUL_DATABUF_SIZE);
    __asm__("sta %v+1", i);

    setup_curbuf_x:
    __asm__("dec %v", i);
    /* load */
    __asm__("ldy #$01");
    __asm__("lda (%v),y", cur_buf_x);
    __asm__("bne %g", not_null_buf);
    __asm__("dey");
    __asm__("lda (%v),y", cur_buf_x);
    __asm__("beq %g", null_buf);
    not_null_buf:

    __asm__("ldx %v+1", val);
    __asm__("stx ptr2+1");
    __asm__("lda %v", val);
    __asm__("sta ptr2");

    __asm__("ldy #$01");
    __asm__("lda (%v),y", cur_buf_x);
    __asm__("tax");
    __asm__("dey");
    __asm__("lda (%v),y", cur_buf_x);

    /* multiply */
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
    /* Then 4 , inlined */
    __asm__("lsr sreg+1");
    __asm__("ror sreg");
    __asm__("ror a");
    __asm__("lsr sreg+1");
    __asm__("ror sreg");
    __asm__("ror a");
    __asm__("lsr sreg+1");
    __asm__("ror sreg");
    __asm__("ror a");
    __asm__("lsr sreg+1");
    __asm__("ror sreg");
    __asm__("ror a");
    __asm__("ldx sreg");
    __asm__("jmp %g", store_buf);

    null_buf:
    __asm__("lda #$0F");
    __asm__("ldx #$00");

    store_buf:
    __asm__("ldy #$00");
    __asm__("sta (%v),y", cur_buf_x);
    __asm__("iny");
    __asm__("txa");
    __asm__("sta (%v),y", cur_buf_x);

    __asm__("clc");
    __asm__("lda %v", cur_buf_x);
    __asm__("adc #2");
    __asm__("sta %v", cur_buf_x);
    __asm__("bcc %g", noof21);
    __asm__("inc %v+1", cur_buf_x);
    noof21:
    __asm__("lda %v", i);
    __asm__("bne %g", setup_curbuf_x);
    __asm__("dec %v+1", i);
    __asm__("bpl %g", setup_curbuf_x);

    __asm__("lda %v", t);
    __asm__("sta %v", last);
#endif
}

void init_top(void) {
  static uint8 l, h;
  l = 0;
  h = 1;
  for (s = i = 0; i != sizeof src; i += 2) {
    r = src[i];
    t = 256 >> r;
    y = src[i+1];
    c = (r << 8 | (uint8) y);
    do {
      huff_split[l][s & 0xFF] = c & 0xFF;
      huff_split[h][s & 0xFF] = c >> 8;
      s++;
      if ((s & 0xFF) == 0) {
        l += 2;
        h += 2;
      }
    } while (--t);
  }

  for (c=0; c != 256; c++) {
    huff_split[18*2][c] = (1284 | c) & 0xFF;
    huff_split[18*2+1][c] = (1284 | c) >> 8;
  }

  cur_buf_x = buf_0;
  for (i=0; i != USEFUL_DATABUF_SIZE; i++) {
    *cur_buf_x = 2048;
    cur_buf_x++;
  }
}

static void decode_row(void) {
#ifndef __CC65__
    for (r=0; r != 2; r++) {
      buf_1[(WIDTH/2)] = (t << 7);
      buf_2[(WIDTH/2)] = (t << 7);
      for (tree = 1, col = (WIDTH/4); col; ) {
        huff_num = tree*2;
        tree = (uint8) getbithuff(8);
        if (tree) {
          col--;
          cur_buf_x = buf_1 + col*2;
          if (tree == 8) {
            huff_num = 18*2;
            tmp8 = (uint8) getbithuff(8);
            *(cur_buf_x+1) = tmp8 * t;
            tmp8 = (uint8) getbithuff(8);
            *cur_buf_x = tmp8 * t;
            cur_buf_x += DATABUF_SIZE;
            tmp8 = (uint8) getbithuff(8);
            *(cur_buf_x+1) = tmp8 * t;
            tmp8 = (uint8) getbithuff(8);
            *cur_buf_x = tmp8 * t;

          } else {
            cur_buf_prev = cur_buf_x - DATABUF_SIZE;

            huff_num = (tree+10)*2;

            y = 2;
            loop2:
            tk = getbithuff(8);
            *(cur_buf_x+1) = ((((*(cur_buf_prev+2) + *(cur_buf_x+2)) >> 1)
                              + *(cur_buf_prev+1)) >> 1)
                             + (tk << 4);

            /* Second with col - 1*/
            tk = getbithuff(8);

            *(cur_buf_x) = ((((*(cur_buf_prev+1) + *(cur_buf_x+1)) >> 1)
                              + *(cur_buf_prev)) >> 1)
                             + (tk << 4);

            y--;
            if (y == 0)
              goto loop2_done;
            cur_buf_prev = cur_buf_x;
            cur_buf_x += DATABUF_SIZE;
            goto loop2;
            loop2_done:
          }
        } else {
          do {
            if (col > 1) {
              huff_num = 9*2;
              nreps = getbithuff(8);
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
            huff_num = 10*2;
            do_rep_loop:
              col--;
              cur_buf_prev = buf_0 + col*2;
              cur_buf_x = cur_buf_prev + DATABUF_SIZE;

              for (y=1; ; y--) {
                /* Unrolled loop3 */

                *(cur_buf_x+1) = ((*(cur_buf_prev+2)
                               + *(cur_buf_x+2)) >> 1)
                               + *(cur_buf_prev+1);
                *(cur_buf_x+1) >>= 1;

                *cur_buf_x = ((*(cur_buf_prev+1)
                           + *(cur_buf_x+1)) >> 1)
                           + *cur_buf_prev;
                *cur_buf_x >>= 1;
                if (!y)
                  break;
                cur_buf_prev = cur_buf_x;
                cur_buf_x += DATABUF_SIZE;
              }
              if (rep & 1) {
                cur_buf_x = cur_buf_prev; /* Previously correctly set */

                tk = getbithuff(8) << 4;

                *cur_buf_x += tk;
                *(cur_buf_x+1) += tk;
                cur_buf_x += DATABUF_SIZE;
                *cur_buf_x += tk;
                *(cur_buf_x+1) += tk;
              }
            rep++;
            if (rep == rep_loop)
              goto rep_loop_done;
            if (!col)
              goto rep_loop_done;
            goto do_rep_loop;

            rep_loop_done:
            
          } while (nreps == 9);
        }
      }
      if (r == 0) {
        raw_ptr1 = row_idx; //FILE_IDX(row, 0);
      } else {
        raw_ptr1 = row_idx_plus2; //FILE_IDX(row + 2, 0);
      }

      cur_buf_x = buf_1;
      for (y=1; ; y++) {
        #define QUARTER_WIDTH (WIDTH/4)
        #if QUARTER_WIDTH != 160
        #error Unexpected width
        #endif

        /* Loop this on Y on 65c02 */
        for (i = 4; i; i--) {
          for (x= 0; x < QUARTER_WIDTH; x+=2) {
            val = *(cur_buf_x+(x/2)) / t;

            if (val > 255)
              val = 255;
            *(raw_ptr1+(x)) = val;
            *(raw_ptr1+(x+1)) = val;
          }
          cur_buf_x+=QUARTER_WIDTH/2;
          raw_ptr1 += QUARTER_WIDTH;
        }
        if (y == 2)
          break;
        cur_buf_x = buf_2;
      }
      memcpy (buf_0+1, buf_2, (USEFUL_DATABUF_SIZE-1)*2);
    }
#else

    __asm__("lda #1");
    __asm__("sta %v", r);
    r_loop:
    // for (r=0; r != 2; r++) {

      // __asm__("ldx #0");
      // __asm__("lda %v", t);
      // __asm__("jsr aslax7");
      /* aslax7 inlined */
      __asm__("lda %v", t);
      __asm__("lsr a");
      __asm__("tax");
      __asm__("lda #0");
      __asm__("ror");

      /* Update buf1/2[WIDTH/2] */
      __asm__("stx %v+%w+1", buf_1, WIDTH);
      __asm__("stx %v+%w+1", buf_2, WIDTH);
      __asm__("sta %v+%w", buf_1, WIDTH);
      __asm__("sta %v+%w", buf_2, WIDTH);

      __asm__("lda #1");
      __asm__("sta %v", tree);

      __asm__("lda #<%w", (WIDTH/4));
      __asm__("sta %v", col);
      __asm__("ldx #0");

      col_loop1:
        __asm__("lda %v", tree);
        __asm__("asl");
        __asm__("clc");
        __asm__("adc #>%v", huff_split);
        __asm__("sta %v", huff_num);

        __asm__("lda #8");
        __asm__("jsr %v", getbithuff);
        __asm__("sta %v", tree);

        __asm__("bne %g", tree_not_zero);
        __asm__("jmp %g", tree_zero);
        tree_not_zero:
          __asm__("dec %v", col);
          __asm__("lda %v", col);

          /* col*2 *2 (uint16) */
          __asm__("ldx #0");
          __asm__("stx tmp1");
          __asm__("asl a");
          __asm__("rol tmp1");
          __asm__("asl a");
          __asm__("rol tmp1");

          __asm__("clc");
          __asm__("adc #<(%v)", buf_1);
          __asm__("sta %v", cur_buf_x);
          __asm__("lda tmp1");
          __asm__("adc #>(%v)", buf_1);
          __asm__("sta %v+1", cur_buf_x);

          __asm__("ldy %v", tree);
          __asm__("cpy #8");
          __asm__("bne %g", tree_not_eight);

          // tree == 8
          __asm__("lda #2");
          __asm__("sta %v", y);

          __asm__("lda #>(%v+18*256*2)", huff_split);
          __asm__("sta %v", huff_num);

          __asm__("lda #8");
          __asm__("jsr %v", getbithuff);
          __asm__("ldx %v", t);
          __asm__("jsr mult8x8r16_direct");
          __asm__("ldy #2");
          __asm__("sta (%v),y", cur_buf_x);
          __asm__("txa");
          __asm__("iny");
          __asm__("sta (%v),y", cur_buf_x);

          __asm__("lda #8");
          __asm__("jsr %v", getbithuff);
          __asm__("ldx %v", t);
          __asm__("jsr mult8x8r16_direct");
          __asm__("ldy #0");
          __asm__("sta (%v),y", cur_buf_x);
          __asm__("txa");
          __asm__("iny");
          __asm__("sta (%v),y", cur_buf_x);

#if DATABUF_SIZE != 0x300
#error Need aligned DATABUF_SIZE
#endif

          __asm__("clc");
          __asm__("lda %v+1", cur_buf_x);
          __asm__("adc #>%w", DATABUF_SIZE);
          __asm__("sta %v+1", cur_buf_x);

          __asm__("lda #8");
          __asm__("jsr %v", getbithuff);
          // __asm__("sta ptr1");
          __asm__("ldx %v", t);
          __asm__("jsr mult8x8r16_direct");
          __asm__("ldy #2");
          __asm__("sta (%v),y", cur_buf_x);
          __asm__("txa");
          __asm__("iny");
          __asm__("sta (%v),y", cur_buf_x);

          __asm__("lda #8");
          __asm__("jsr %v", getbithuff);
          // __asm__("sta ptr1");
          __asm__("ldx %v", t);
          __asm__("jsr mult8x8r16_direct");
          __asm__("ldy #0");
          __asm__("sta (%v),y", cur_buf_x);
          __asm__("txa");
          __asm__("iny");
          __asm__("sta (%v),y", cur_buf_x);

          __asm__("jmp %g", tree_done);

          tree_not_eight:
            /* set cur_buf_prev from cur_buf_x */
            __asm__("lda %v+1", cur_buf_x);
            __asm__("sec");
            __asm__("sbc #>%w", DATABUF_SIZE);
            __asm__("sta %v+1", cur_buf_prev);
            __asm__("lda %v", cur_buf_x);
            __asm__("sta %v", cur_buf_prev);

            //huff_ptr = huff[tree + 10];
            __asm__("lda %v", tree);
            __asm__("asl a");
            __asm__("adc #>(%v+10*256*2)", huff_split);
            __asm__("sta %v", huff_num);

            __asm__("lda #2");
            __asm__("sta %v", y);
            loop2:
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            // __asm__("sta %v", tk);

            __asm__("ldx #0");
            __asm__("cmp #0");
            __asm__("bpl %g", pos1);
            __asm__("dex");
            pos1:
            __asm__("stx tmp4");  // tk << 4
            __asm__("asl a");
            __asm__("rol tmp4");
            __asm__("asl a");
            __asm__("rol tmp4");
            __asm__("asl a");
            __asm__("rol tmp4");
            __asm__("asl a");
            __asm__("rol tmp4");
            __asm__("sta tmp3");

            __asm__("clc");
            __asm__("ldy #4");
            __asm__("lda (%v),y", cur_buf_prev);
            __asm__("adc (%v),y", cur_buf_x);
            __asm__("tax");
            __asm__("ldy #5");
            __asm__("lda (%v),y", cur_buf_prev);
            __asm__("adc (%v),y", cur_buf_x);
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            __asm__("clc");
            __asm__("ldy #2");
            __asm__("adc (%v),y", cur_buf_prev);
            __asm__("tax");
            __asm__("lda tmp1");
            __asm__("ldy #3");
            __asm__("adc (%v),y", cur_buf_prev);
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            __asm__("clc");
            __asm__("adc tmp3");
            __asm__("ldy #2");
            __asm__("sta (%v),y", cur_buf_x);

            __asm__("lda tmp1");
            __asm__("adc tmp4");
            __asm__("ldy #3");
            __asm__("sta (%v),y", cur_buf_x);

            /* Second with col - 1*/
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            // __asm__("sta %v", tk);

            __asm__("ldx #0");
            __asm__("cmp #0");
            __asm__("bpl %g", pos2);
            __asm__("dex");
            pos2:
            __asm__("stx tmp4");  // tk << 4
            __asm__("asl a");
            __asm__("rol tmp4");
            __asm__("asl a");
            __asm__("rol tmp4");
            __asm__("asl a");
            __asm__("rol tmp4");
            __asm__("asl a");
            __asm__("rol tmp4");
            __asm__("sta tmp3");

            __asm__("clc");
            __asm__("ldy #2");
            __asm__("lda (%v),y", cur_buf_prev);
            __asm__("adc (%v),y", cur_buf_x);
            __asm__("tax");
            __asm__("ldy #3");
            __asm__("lda (%v),y", cur_buf_prev);
            __asm__("adc (%v),y", cur_buf_x);
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            __asm__("clc");
            __asm__("ldy #0");
            __asm__("adc (%v),y", cur_buf_prev);
            __asm__("tax");
            __asm__("lda tmp1");
            __asm__("iny");
            __asm__("adc (%v),y", cur_buf_prev);
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            /* Store to cur_buf_x */
            __asm__("clc");
            __asm__("adc tmp3");
            __asm__("ldy #0");
            __asm__("sta (%v),y", cur_buf_x);

            __asm__("lda tmp1");
            __asm__("iny");
            __asm__("adc tmp4");
            __asm__("sta (%v),y", cur_buf_x);

            __asm__("dec %v", y);
            __asm__("beq %g", tree_done);

            /* shift pointers */
            __asm__("clc");
            __asm__("lda %v+1", cur_buf_x);
            __asm__("sta %v+1", cur_buf_prev);
            __asm__("adc #>%w", DATABUF_SIZE);
            __asm__("sta %v+1", cur_buf_x);

            __asm__("jmp %g", loop2);

        tree_zero:
          nine_reps_loop:
            __asm__("lda %v", col);
            __asm__("cmp #2");
            __asm__("bcs %g", col_gt1a);
            __asm__("lda #1"); /* nreps */
            __asm__("jmp %g", check_nreps);
            col_gt1a:
            __asm__("lda #>(%v+9*256*2)", huff_split);
            __asm__("sta %v", huff_num);
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            __asm__("clc");
            __asm__("adc #1");
            check_nreps:
            __asm__("sta %v", nreps);

            __asm__("cmp #9");
            __asm__("bcc %g", nreps_check_done);
            __asm__("lda #8");
            nreps_check_done:
            __asm__("sta %v", rep_loop);

            __asm__("ldy #$00");
            __asm__("sty %v", rep);

            // __asm__("lda %v", huff_10);
            // __asm__("sta %v", huff_ptr);
            __asm__("lda #>(%v+10*256*2)", huff_split);
            __asm__("sta %v", huff_num);
            do_rep_loop:
              __asm__("dec %v", col);
              __asm__("lda %v", col);
              __asm__("ldx #0");
              __asm__("stx tmp1");

              /* set cur_buf_prev */
              /* set cur_buf_x (prevy in YX) */
              __asm__("asl a");
              __asm__("rol tmp1");
              __asm__("asl a");
              __asm__("rol tmp1");

              __asm__("clc");
              __asm__("adc #<(%v)", buf_0);
              __asm__("sta %v", cur_buf_prev);
              __asm__("sta %v", cur_buf_x);
              __asm__("lda tmp1");
              __asm__("adc #>(%v)", buf_0);
              __asm__("sta %v+1", cur_buf_prev);
              // __asm__("clc"); we better not overflow there
              __asm__("adc #>%w", DATABUF_SIZE);
              __asm__("sta %v+1", cur_buf_x);

              __asm__("lda #2");
              __asm__("sta %v", y);

              loop3:
              __asm__("clc");
              __asm__("ldy #4");
              __asm__("lda (%v),y", cur_buf_prev);
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("tax");
              __asm__("ldy #5");
              __asm__("lda (%v),y", cur_buf_prev);
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("ror a");
              __asm__("sta tmp1");
              __asm__("txa");
              __asm__("ror a");

              __asm__("clc");
              __asm__("ldy #2");
              __asm__("adc (%v),y", cur_buf_prev);
              __asm__("tax");
              __asm__("lda tmp1");
              __asm__("iny");
              __asm__("adc (%v),y", cur_buf_prev);
              __asm__("ror a");
              __asm__("sta (%v),y", cur_buf_x);
              __asm__("txa");
              __asm__("ror a");
              __asm__("dey");
              __asm__("sta (%v),y", cur_buf_x);

              /* Second */
              __asm__("clc");
              // __asm__("ldy #2");
              __asm__("lda (%v),y", cur_buf_prev);
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("tax");
              __asm__("iny");
              __asm__("lda (%v),y", cur_buf_prev);
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("ror a");
              __asm__("sta tmp1");
              __asm__("txa");
              __asm__("ror a");

              __asm__("clc");
              __asm__("ldy #0");
              __asm__("adc (%v),y", cur_buf_prev);
              __asm__("tax");
              __asm__("lda tmp1");
              __asm__("iny");
              __asm__("adc (%v),y", cur_buf_prev);
              __asm__("ror a");
              __asm__("sta (%v),y", cur_buf_x);
              __asm__("txa");
              __asm__("ror a");
              __asm__("dey");
              __asm__("sta (%v),y", cur_buf_x);

              __asm__("dec %v", y);
              __asm__("beq %g", loop3_done);

              /* shift pointers */
              __asm__("clc");
              __asm__("lda %v+1", cur_buf_x);
              __asm__("sta %v+1", cur_buf_prev);
              __asm__("adc #>%w", DATABUF_SIZE);
              __asm__("sta %v+1", cur_buf_x);

              __asm__("jmp %g", loop3);
              loop3_done:

              __asm__("lda %v", rep);
              __asm__("and #1");
              __asm__("beq %g", rep_even);

              // cur_buf_x = cur_buf_prev;
              // __asm__("lda %v", cur_buf_prev); - aligned so no need
              // __asm__("sta %v", cur_buf_x);
              __asm__("lda %v+1", cur_buf_prev);
              __asm__("sta %v+1", cur_buf_x);

              // tk = getbithuff(8) << 4;
              __asm__("lda #8");
              __asm__("jsr %v", getbithuff);

              __asm__("ldx #0");
              __asm__("cmp #0");
              __asm__("bpl %g", pos3);
              __asm__("dex");
              pos3:
              __asm__("stx tmp4");
              __asm__("asl a");
              __asm__("rol tmp4");
              __asm__("asl a");
              __asm__("rol tmp4");
              __asm__("asl a");
              __asm__("rol tmp4");
              __asm__("asl a");
              __asm__("rol tmp4");
              __asm__("tax");

              // *cur_buf_x += tk;
              __asm__("clc");
              // __asm__("txa");
              __asm__("ldy #0");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);
              __asm__("lda tmp4");
              __asm__("iny");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);

              // *(cur_buf_x+1) += tk;
              __asm__("clc");
              __asm__("txa");
              __asm__("iny");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);
              __asm__("lda tmp4");
              __asm__("iny");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);

              /* shift pointer */
              __asm__("clc");
              __asm__("lda %v+1", cur_buf_x);
              __asm__("adc #>%w", DATABUF_SIZE);
              __asm__("sta %v+1", cur_buf_x);

              // *cur_buf_x += tk;
              __asm__("clc");
              __asm__("txa");
              __asm__("ldy #0");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);
              __asm__("lda tmp4");
              __asm__("iny");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);

              // *(cur_buf_x+1) += tk;
              __asm__("clc");
              __asm__("txa");
              __asm__("iny");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);
              __asm__("lda tmp4");
              __asm__("iny");
              __asm__("adc (%v),y", cur_buf_x);
              __asm__("sta (%v),y", cur_buf_x);

            rep_even:
            __asm__("ldx %v", rep);
            __asm__("inx");
            __asm__("cpx %v", rep_loop);
            __asm__("beq %g", rep_loop_done);
            __asm__("stx %v", rep);
            __asm__("lda %v", col);
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
      __asm__("beq %g", col_loop1_done);
      __asm__("jmp %g", col_loop1);
      col_loop1_done:

      __asm__("clc");

      __asm__("ldx %v", r);
      /* logic inverted from C because here we dec R */
      __asm__("beq %g", store_plus_2);

      __asm__("lda %v", row_idx);
      __asm__("sta %v", raw_ptr1);
      __asm__("lda %v+1", row_idx);
      __asm__("sta %v+1", raw_ptr1);
      goto store_set;
      store_plus_2:
      __asm__("lda %v", row_idx_plus2);
      __asm__("sta %v", raw_ptr1);
      __asm__("lda %v+1", row_idx_plus2);
      __asm__("sta %v+1", raw_ptr1);

      store_set:
      __asm__("lda #2");
      __asm__("sta %v", y);

      __asm__("ldy #<(%v)", buf_1);
      __asm__("sty %v", cur_buf_x);
      __asm__("lda #>(%v)", buf_1);
      __asm__("sta %v+1", cur_buf_x);

      loop5:
        __asm__("lda #4");
        __asm__("sta %v", x);
        x_loop_outer:
        __asm__("ldy #<%b", 0);
        x_loop:
#if 0 // define for precise divisions
          __asm__("iny");
          __asm__("lda (%v),y", cur_buf_x);
          __asm__("tax");
          __asm__("dey");
          __asm__("lda (%v),y", cur_buf_x);
          __asm__("sty tmp1");
          __asm__("jsr pushax");
          __asm__("lda %v", t);
          __asm__("jsr tosudiva0");
          __asm__("ldy tmp1");
#else
          __asm__("iny");
          __asm__("lda (%v),y", cur_buf_x);
          __asm__("tax");
          __asm__("dey");
          __asm__("lda (%v),y", cur_buf_x);
          __asm__("sty tmp1");
          __asm__("ldy %v", t);
          __asm__("jsr approx_div16x8_direct");
          __asm__("ldy tmp1");
#endif
          __asm__("cpx #0");
          __asm__("beq %g", no_val_clamp);
          __asm__("lda #$FF");

          no_val_clamp:
          __asm__("sta (%v),y", raw_ptr1);

        __asm__("iny");
        __asm__("iny");
        __asm__("cpy #<%b", (WIDTH/4));
        __asm__("bne %g", x_loop);

        __asm__("clc");
        __asm__("lda %v", cur_buf_x);
        __asm__("adc #<%w", (WIDTH/4));
        __asm__("sta %v", cur_buf_x);
        __asm__("lda %v+1", cur_buf_x);
        __asm__("adc #>%w", (WIDTH/4));
        __asm__("sta %v+1", cur_buf_x);

        __asm__("clc");
        __asm__("lda %v", raw_ptr1);
        __asm__("adc #<%w", (WIDTH/4));
        __asm__("sta %v", raw_ptr1);
        __asm__("lda %v+1", raw_ptr1);
        __asm__("adc #>%w", (WIDTH/4));
        __asm__("sta %v+1", raw_ptr1);

        __asm__("dec %v", x);
        __asm__("bne %g", x_loop_outer);

        __asm__("dec %v", y);
        __asm__("beq %g", loop5_done);

        __asm__("inc %v", raw_ptr1);
        __asm__("bne %g", noof22);
        __asm__("inc %v+1", raw_ptr1);
        noof22:

      __asm__("ldy #<(%v)", buf_2);
      __asm__("sty %v", cur_buf_x);
      __asm__("lda #>(%v)", buf_2);
      __asm__("sta %v+1", cur_buf_x);

        __asm__("jmp %g", loop5);

      loop5_done:
      __asm__("clc");
      __asm__("ldx #>(%v+2)", buf_0); /* cur_buf[0]+1 */
      __asm__("lda #<(%v+2)", buf_0);
      __asm__("jsr pushax");

      __asm__("lda #<(%v)", buf_2); /* curbuf_2 */
      __asm__("ldx #>(%v)", buf_2);
      __asm__("jsr pushax");

      __asm__("lda #<%w", 2*(USEFUL_DATABUF_SIZE-1));
      __asm__("ldx #>%w", 2*(USEFUL_DATABUF_SIZE-1));
      __asm__("jsr _memcpy");
    // }
    __asm__("dec %v", r);
    __asm__("bmi %g", r_loop_done);
    __asm__("jmp %g", r_loop);
    r_loop_done:
    return;
#endif
}

static void consume_extra(void) {
#ifndef __CC65__
    /* Consume RADC tokens but don't care about them. */
    for (c=1; c != 3; c++) {
      for (tree = 1, col = WIDTH/4; col; ) {
        huff_num = tree*2;
        tree = getbithuff(8);
        if (tree) {
          col--;
          huff_num = (tree + 10)*2;
          getbithuff(8);
          getbithuff(8);
          getbithuff(8);
          getbithuff(8);
        } else {
          do {
            if (col > 1) {
              huff_num = 9*2;
              nreps = getbithuff(8) + 1;
            } else {
              nreps = 1;
            }
            if (nreps > 8) {
              rep_loop = 8;
            } else {
              rep_loop = nreps;
            }
            huff_num = 10*2;
            for (rep=0; rep != rep_loop && col; rep++) {
              col--;
              if (rep & 1) {
                getbithuff(8);
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
      __asm__("ldy #$00");
      __asm__("sty %v+1", tree);
      __asm__("lda #<%w", (WIDTH/4));
      __asm__("sta %v", col);

      col_loop2:
        __asm__("lda %v", tree);
        __asm__("asl a");
        __asm__("clc");
        __asm__("adc #>%v", huff_split);
        __asm__("sta %v", huff_num);
        // __asm__("lda #<%v", huff);
        // __asm__("sta %v", huff_ptr);

        __asm__("lda #8");
        __asm__("jsr %v", getbithuff);
        __asm__("sta %v", tree);

        __asm__("beq %g", tree_zero_2);
        tree_not_zero_2:
          __asm__("dec %v", col);

          //huff_ptr = huff[tree + 10];
          __asm__("lda %v", tree);
          __asm__("clc");
          __asm__("adc #10");
          __asm__("asl a");
          __asm__("adc #>%v", huff_split);
          __asm__("sta %v", huff_num);
          // __asm__("lda #<%v", huff);
          // __asm__("sta %v", huff_ptr);

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
            __asm__("cmp #2");
            __asm__("bcs %g", col_gt1);

            __asm__("lda #1"); /* nreps */
            __asm__("jmp %g", check_nreps_2);

            col_gt1:
            // __asm__("lda %v", huff_9);
            // __asm__("sta %v", huff_ptr);
            __asm__("lda #>(%v+9*256*2)", huff_split);
            __asm__("sta %v", huff_num);
            __asm__("lda #8");
            __asm__("jsr %v", getbithuff);
            __asm__("clc");
            __asm__("adc #1");

            check_nreps_2:
            __asm__("sta %v", nreps);

            __asm__("cmp #9");
            __asm__("bcc %g", nreps_check_done_2);
            __asm__("lda #8");
            nreps_check_done_2:
            __asm__("sta %v", rep_loop);

            // __asm__("lda %v", huff_10);
            // __asm__("sta %v", huff_ptr);
            __asm__("lda #>(%v+10*256*2)", huff_split);
            __asm__("sta %v", huff_num);

            __asm__("ldy #$00");
            __asm__("sty %v", rep);
            do_rep_loop_2:
            __asm__("dec %v", col);

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

            rep_loop_2_done:
          __asm__("lda %v", nreps);
          __asm__("cmp #9");
          __asm__("beq %g", tree_zero_2);
        tree_zero_2_done:
      __asm__("lda %v", col);
      __asm__("bne %g", col_loop2);

    __asm__("dec %v", c);
    __asm__("beq %g", c_loop_done);
    __asm__("jmp %g", c_loop);
    c_loop_done:
#endif
  return;
}

static void copy_data(void) {
#ifndef __CC65__
    raw_ptr1 = row_idx - 1;
    for (y = 4; y; y--) {
      int steps, i, j;
      x = (y & 1);
      if (x) {
        steps = (WIDTH/4)-2;
        x = 0;
      } else {
        steps = (WIDTH/4)-1;
        x = 1;
      }

      for (j = 4; j; j--) {
        for (i = x; i != steps; i+=2) {
          *(raw_ptr1 + (i+1)) = *(raw_ptr1+i);
        }
        raw_ptr1 += (WIDTH/4);
      }
    }

    row_idx += (WIDTH<<2);
    row_idx_plus2 += (WIDTH<<2);

#else
    //raw_ptr1 = row_idx - 1;
    __asm__("clc");
    __asm__("ldx %v+1", row_idx);
    __asm__("ldy %v", row_idx);
    __asm__("bne %g", nouf19);
    __asm__("dex");
    nouf19:
    __asm__("dey");
    __asm__("sty %v", raw_ptr1);
    __asm__("stx %v+1", raw_ptr1);

    __asm__("ldy #4");
    __asm__("sty %v", y);
    y_loop:

    __asm__("ldx #4");
    copy_loop:
    __asm__("lda %v", y);
    __asm__("and #1");
    __asm__("beq %g", even_y);
    __asm__("lda #<%b", (WIDTH/4)-2);
    __asm__("sta tmp1");
    __asm__("ldy #0");
    __asm__("jmp %g", inner_copy_loop);
    even_y:
    __asm__("lda #<%b", (WIDTH/4)-1);
    __asm__("ldy #1");
    __asm__("sta tmp1");
    inner_copy_loop:
    __asm__("lda (%v),y", raw_ptr1);
    __asm__("iny");
    __asm__("sta (%v),y", raw_ptr1);
    __asm__("iny");
    __asm__("cpy tmp1");
    __asm__("bne %g", inner_copy_loop);

    __asm__("clc");
    __asm__("lda %v", raw_ptr1);
    __asm__("adc #<%w", (WIDTH/4));
    __asm__("sta %v", raw_ptr1);
    __asm__("lda %v+1", raw_ptr1);
    __asm__("adc #>%w", (WIDTH/4));
    __asm__("sta %v+1", raw_ptr1);

    __asm__("dex");
    __asm__("bne %g", copy_loop);

    /* Finish Y loop */
    __asm__("dec %v", y);
    __asm__("lda %v", y);
    __asm__("bne %g", y_loop);

    __asm__("clc");
    __asm__("lda %v", row_idx);
    __asm__("adc #<%w", (WIDTH<<2));
    __asm__("sta %v", row_idx);
    __asm__("lda %v+1", row_idx);
    __asm__("adc #>%w", (WIDTH<<2));
    __asm__("sta %v+1", row_idx);

    __asm__("lda %v", row_idx_plus2);
    __asm__("adc #<%w", (WIDTH<<2));
    __asm__("sta %v", row_idx_plus2);
    __asm__("lda %v+1", row_idx_plus2);
    __asm__("adc #>%w", (WIDTH<<2));
    __asm__("sta %v+1", row_idx_plus2);
#endif
}

void qt_load_raw(uint16 top)
{
  if (top == 0) {
#ifdef __CC65__
    init_floppy_starter();
#endif
    /* Init */
    init_top();

    if (width != 640) {
      cprintf("Unsupported format\r\n");
      return;
    }
  }

  #ifdef __CC65__
  huff_num = 0;
  #else
  huff_num = 255;
  #endif

  row_idx = raw_image;
  row_idx_plus2 = raw_image + (WIDTH*2);

  for (row=0; row != BAND_HEIGHT; row+=4) {
    progress_bar(-1, -1, 80*22, (top + row), height);
    #ifdef __CC65__
    huff_num = 0;
    #else
    huff_num = 255;
    #endif
    t = getbithuff(6);
    /* Ignore those */
    getbithuff(6);
    getbithuff(6);

    c = 0;

    val = val_from_last[last] * t;

    cur_buf_x = buf_0;

    init_row();

    decode_row();
    consume_extra();

    copy_data();
  }
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
