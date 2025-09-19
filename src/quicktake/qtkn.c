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

static uint8 val_hi_from_last[17] = {
  0x00, 0x10, 0x08, 0x05, 0x04, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
};
static uint8 val_from_last[256] = {
  0x00, 0x00, 0x00, 0x55, 0x00, 0x33, 0xab, 0x49, 0x00, 0xc7, 0x9a, 0x74, 0x55, 0x3b, 0x25, 0x11, 0x00,
  0xf1, 0xe4, 0xd8, 0xcd, 0xc3, 0xba, 0xb2, 0xab, 0xa4, 0x9e, 0x98, 0x92, 0x8d, 0x89, 0x84, 0x80,
  0x7c, 0x78, 0x75, 0x72, 0x6f, 0x6c, 0x69, 0x66, 0x64, 0x62, 0x5f, 0x5d, 0x5b, 0x59, 0x57, 0x55,
  0x54, 0x52, 0x50, 0x4f, 0x4d, 0x4c, 0x4a, 0x49, 0x48, 0x47, 0x45, 0x44, 0x43, 0x42, 0x41, 0x40,
  0x3f, 0x3e, 0x3d, 0x3c, 0x3b, 0x3b, 0x3a, 0x39, 0x38, 0x37, 0x37, 0x36, 0x35, 0x35, 0x34, 0x33,
  0x33, 0x32, 0x31, 0x31, 0x30, 0x30, 0x2f, 0x2f, 0x2e, 0x2e, 0x2d, 0x2d, 0x2c, 0x2c, 0x2b, 0x2b,
  0x2a, 0x2a, 0x29, 0x29, 0x29, 0x28, 0x28, 0x27, 0x27, 0x27, 0x26, 0x26, 0x26, 0x25, 0x25, 0x25,
  0x24, 0x24, 0x24, 0x23, 0x23, 0x23, 0x22, 0x22, 0x22, 0x22, 0x21, 0x21, 0x21, 0x21, 0x20, 0x20,
  0x20, 0x20, 0x1f, 0x1f, 0x1f, 0x1f, 0x1e, 0x1e, 0x1e, 0x1e, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1c,
  0x1c, 0x1c, 0x1c, 0x1c, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1b, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a,
  0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x17, 0x17,
  0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x15, 0x15,
  0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
  0x14, 0x14, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x12, 0x12, 0x12,
  0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
  0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
};

/* note that each huff[x] share the same low byte addr */
static int16 x, s, i;
static uint16 c;
static uint8 r, nreps, row, y, t, rep_loop, tree;

/* Wastes bytes, but simplifies adds */

static uint16 val;
#ifndef __CC65__
static int8 tk;
static uint16 tk1, tk2, tk3, tk4;
static uint8 tmp8;
static uint16 tmp16;
static uint32 tmp32;
static uint8 shiftl4p_l[128];
static uint8 shiftl4p_h[128];
static uint8 shiftl4n_l[128];
static uint8 shiftl4n_h[128];
uint8 shiftl3[32];
#endif

#ifdef __CC65__
#define USEFUL_DATABUF_SIZE 321
#define DATABUF_SIZE 0x400 /* align things */
extern uint8 buf_0[DATABUF_SIZE];
extern uint8 buf_1[DATABUF_SIZE];
extern uint8 buf_2[DATABUF_SIZE];

extern uint8 shiftl4p_l[128];
extern uint8 shiftl4p_h[128];
extern uint8 shiftl4n_l[128];
extern uint8 shiftl4n_h[128];
extern uint8 shiftl3[32];

extern uint8 huff_split[18*2][256];
extern uint8 huff_num, huff_num_h;

#define raw_ptr1 zp2ip
#define cur_buf_0l zp4p
#define cur_buf_0h zp6p
#define col zp7
static uint8 colh;
// zp8-12 used by bitbuffer
#define rep zp13

#else
#define USEFUL_DATABUF_SIZE 321
#define DATABUF_SIZE 0x400 /* align things */
static uint8 buf_0[DATABUF_SIZE];
static uint8 buf_1[DATABUF_SIZE];
static uint8 buf_2[DATABUF_SIZE];
static uint16 col;
uint8 huff_split[18*2][256];
uint8 huff_num;
uint8 rep;
static uint8 *raw_ptr1;

static uint8 *cur_buf_0l, *cur_buf_1l, *cur_buf_2l;
static uint8 *cur_buf_0h, *cur_buf_1h, *cur_buf_2h;

#endif
static uint8 *row_idx, *row_idx_plus2;

static const uint8 src[] = {
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

#define SET_CURBUF_VAL(bufl, bufh, y, val) do { int16 v = (int16)(val); *(uint8 *)((bufl)+(y)) = (v)&0xff; *(uint8 *)((bufh)+(y)) = (v)>>8; } while (0)
#define GET_CURBUF_VAL(bufl, bufh, y) ((int16)(((uint8)( *((bufl)+(y)) ))|(((uint8)( *(((bufh))+(y)) ))<<8)))

static void init_row(void) {
#ifndef __CC65__
    if (last > 17)
      val = (val_from_last[last] * t) >> 4;
    else
      val = ((val_from_last[last]|(val_hi_from_last[last]<<8)) * t) >> 4;
    last = t;
    cur_buf_0l = buf_0;
    cur_buf_0h = buf_0+(DATABUF_SIZE/2);
    for (i=USEFUL_DATABUF_SIZE-1; i >=0; i--) {
      tmp32 = val;
      if (GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 0)) {
        tmp32 *= GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 0);
        tmp32--;
        SET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 0, tmp32 >> 8);
      } else {
        // tmp32 *= 0  == 0
        // tmp32--     == 0xFFFF
        // tmp32 >> 12 == 0x0F
        SET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 0, 0x0F);
      }
      cur_buf_0l++;
      cur_buf_0h++;
    }

#else
    __asm__("lda #<(%v+256)", buf_0);
    __asm__("ldx #>(%v+256)", buf_0);
    __asm__("sta %v", cur_buf_0l);
    __asm__("stx %v+1", cur_buf_0l);
    __asm__("lda #<(%v+256+512)", buf_0);
    __asm__("ldx #>(%v+256+512)", buf_0);
    __asm__("sta %v", cur_buf_0h);
    __asm__("stx %v+1", cur_buf_0h);

    __asm__("ldy %v", last);
    __asm__("cpy #18");
    __asm__("bcs %g", small_val);

    __asm__("lda %v,y", val_from_last);
    __asm__("ldx %v,y", val_hi_from_last);
    __asm__("jsr pushax");
    __asm__("lda %v", t);
    __asm__("sta %v", last);
    __asm__("jsr tosmula0");
    goto shift_val;
    small_val:
    __asm__("lda %v,y", val_from_last);
    __asm__("ldx %v", t);
    __asm__("stx %v", last);
    __asm__("jsr mult8x8r16_direct");
    shift_val:
    __asm__("stx ptr2+1");
    __asm__("lsr ptr2+1");
    __asm__("ror a");
    __asm__("lsr ptr2+1");
    __asm__("ror a");
    __asm__("lsr ptr2+1");
    __asm__("ror a");
    __asm__("lsr ptr2+1");
    __asm__("ror a");
    __asm__("sta ptr2");

    __asm__("ldy #<%w", USEFUL_DATABUF_SIZE);
    __asm__("sty %v", i);
    __asm__("lda #>%w", USEFUL_DATABUF_SIZE);
    __asm__("sta %v+1", i);

    setup_curbuf_x:
    /* load */
    __asm__("dey");
    __asm__("sty %v", i);
    __asm__("lda (%v),y", cur_buf_0h);
    __asm__("tax");
    __asm__("bne %g", not_null_buf);
    __asm__("lda (%v),y", cur_buf_0l);
    __asm__("beq %g", null_buf);
    not_null_buf:

    __asm__("lda (%v),y", cur_buf_0l);

    /* multiply */
    __asm__("jsr mult16x16r24_direct");

    /* Shift >> 8 */
    __asm__("txa");
    __asm__("jmp %g", store_buf);

    null_buf:
    __asm__("lda #$0F");
    __asm__("ldx #$00");
    __asm__("stx sreg");

    store_buf:
    __asm__("ldy %v", i);
    __asm__("sta (%v),y", cur_buf_0l);
    __asm__("lda sreg");
    __asm__("sta (%v),y", cur_buf_0h);

    __asm__("ldy %v", i);
    __asm__("bne %g", setup_curbuf_x);
    __asm__("dec %v+1", cur_buf_0l);
    __asm__("dec %v+1", cur_buf_0h);
    __asm__("dec %v+1", i);
    __asm__("bpl %g", setup_curbuf_x);
#endif
}

void init_top(void) {
  static uint8 l, h;

  l = 0;
  h = 1;
  for (s = i = 0; i != sizeof src; i += 2) {
    uint8 code = s & 0xFF;
    r = src[i];
    t = 256 >> r;

    code >>= 8-r;
    huff_split[l][code] = src[i+1];
    huff_split[h][code] = r;
    // printf("huff[%d][%.*b] = %d (r%d)\n", l, r, code, y, r);

    if (s >> 8 != (s+t) >> 8) {
      l += 2;
      h += 2;
    }
    s += t;
  }

  for (c=0; c != 32; c++) {
    shiftl3[c] = (c<<3)+4;
    // printf("huff[%d][%.*b] = %d (r%d)\n", 36, 5, c, (c<<3)+4, 5);
  }

  for (c = 0; c < 256; c++) {
    int8 sc = (int8)c;
    if (sc >= 0) {
      shiftl4p_l[c] = (sc<<4) & 0xFF;
      shiftl4p_h[c] = (sc<<4) >> 8;
      // printf("l4 p[%02X] = %04X\n", c, (uint16)(sc<<4));
    } else {
      shiftl4n_l[c-128] = ((int16)(sc<<4)) & 0xFF;
      shiftl4n_h[c-128] = ((int16)(sc<<4)) >> 8;
      // printf("l4 n[%02X] = %04X [%02X%02X]\n", c-128, (uint16)(sc<<4),
      //        shiftl4n_h[c-128], shiftl4n_l[c-128]);
    }
  }

  cur_buf_0l = buf_0;
  cur_buf_0h = buf_0+(DATABUF_SIZE/2);
  for (i=0; i != USEFUL_DATABUF_SIZE; i++) {
    SET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 0, 2048);
    cur_buf_0l++;
    cur_buf_0h++;
  }
}

static void decode_row(void) {
#ifndef __CC65__
    for (r=0; r != 2; r++) {
      SET_CURBUF_VAL(buf_1, buf_1+(DATABUF_SIZE/2), (WIDTH/2), (t<<7));
      SET_CURBUF_VAL(buf_2, buf_2+(DATABUF_SIZE/2), (WIDTH/2), (t<<7));

      col = WIDTH/2;
      cur_buf_0l = buf_0 + col;
      cur_buf_0h = cur_buf_0l+(DATABUF_SIZE/2);
      cur_buf_1l = cur_buf_0l + DATABUF_SIZE;
      cur_buf_1h = cur_buf_1l+(DATABUF_SIZE/2);
      cur_buf_2l = cur_buf_1l + DATABUF_SIZE;
      cur_buf_2h = cur_buf_2l+(DATABUF_SIZE/2);
      tree = 1;

      while(col) {
        huff_num = tree*2;
        tree = (uint8) getbithuff();

        if (tree) {
          col-=2;
          cur_buf_0l-=2;
          cur_buf_0h-=2;
          cur_buf_1l-=2;
          cur_buf_1h-=2;
          cur_buf_2l-=2;
          cur_buf_2h-=2;

          if (tree == 8) {
            tmp8 = (uint8) getbithuff36();
            SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1, tmp8 * t);
            tmp8 = (uint8) getbithuff36();
            SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0, tmp8 * t);
            tmp8 = (uint8) getbithuff36();
            SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1, tmp8 * t);
            tmp8 = (uint8) getbithuff36();
            SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0, tmp8 * t);
          } else {
            huff_num = (tree+10)*2;

            //a
            tk1 = ((int8)getbithuff()) << 4;
            tk2 = ((int8)getbithuff()) << 4;
            tk3 = ((int8)getbithuff()) << 4;
            tk4 = ((int8)getbithuff()) << 4;
            SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1, 
                            (((((GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 2) + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 2)) >> 1)
                              + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 1)) >> 1)
                              + tk1));

            /* Second with col - 1*/
            SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0, 
                            (((((GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1) + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 1)) >> 1)
                              + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 0)) >> 1)
                              + tk2));

            //b
            SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1, 
                            (((((GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 2) + GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 2)) >> 1)
                              + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)) >> 1)
                              + tk3));

            /* Second with col - 1*/
            SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0, 
                            (((((GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1) + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)) >> 1)
                              + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0)) >> 1)
                              + tk4));
          }
        } else {
          do {
            if (col > 2) {
              huff_num = 9*2;
              nreps = getbithuff();
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
              col-=2;
              cur_buf_0l-=2;
              cur_buf_0h-=2;
              cur_buf_1l-=2;
              cur_buf_1h-=2;
              cur_buf_2l-=2;
              cur_buf_2h-=2;

              //c
              SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1,
                             (((GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 2)
                             + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 2)) >> 1)
                             + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 1)) >> 1);

              SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0,
                             (((GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)
                             + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 1)) >> 1)
                             + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 0)) >> 1);

              //d
              SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1,
                             (((GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 2)
                             + GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 2)) >> 1)
                             + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)) >> 1);

              SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0,
                             (((GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1)
                             + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)) >> 1)
                             + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0)) >> 1);

              if (rep & 1) {
                tk = getbithuff() << 4;
                //e
                SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0, GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0)+tk);
                SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1, GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)+tk);
                SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0, GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0)+tk);
                SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1, GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1)+tk);
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

      cur_buf_1l = buf_1;
      cur_buf_1h = cur_buf_1l + (DATABUF_SIZE/2);
      
      for (y=1; ; y++) {
        #define QUARTER_WIDTH (WIDTH/4)
        #if QUARTER_WIDTH != 160
        #error Unexpected width
        #endif
      
        /* Loop this on Y on 65c02 */
        for (i = 4; i; i--) {
          for (x= 0; x < QUARTER_WIDTH; x+=2) {
            if (cur_buf_1h[x/2] & 0x80) {
              val = 0;
            } else {
              val = GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, x/2) / t;
              if (val > 255)
                val = 255;
            }
            *(raw_ptr1+(x)) = val;
            *(raw_ptr1+(x+1)) = val;
          }
          cur_buf_1l+=QUARTER_WIDTH/2;
          cur_buf_1h = cur_buf_1l + (DATABUF_SIZE/2);
          raw_ptr1 += QUARTER_WIDTH;
        }
        if (y == 2)
          break;
        cur_buf_1l = buf_2;
        cur_buf_1h = cur_buf_1l + (DATABUF_SIZE/2);
      }
      memcpy (buf_0+1, buf_2, (USEFUL_DATABUF_SIZE-1));
      memcpy (buf_0+512+1, buf_2+512, (USEFUL_DATABUF_SIZE-1));
    }
#else

    __asm__("lda #1");
    __asm__("sta %v", r);
    r_loop:
    // for (r=0; r != 2; r++) {
      /* t<<7, aslax7 inlined */
      __asm__("lda %v", t);
      __asm__("lsr a");
      __asm__("tax");
      __asm__("lda #0");
      __asm__("ror");

      /* Update buf1/2[WIDTH/2] = t<<7 */
      __asm__("stx %v+%w+512", buf_1, WIDTH/2);
      __asm__("stx %v+%w+512", buf_2, WIDTH/2);
      __asm__("sta %v+%w", buf_1, WIDTH/2);
      __asm__("sta %v+%w", buf_2, WIDTH/2);

      __asm__("lda #2");
      __asm__("sta %v", tree);

      __asm__("lda #<%w", (WIDTH/2));
      __asm__("sta %v", col);
      __asm__("ldx #>%w", (WIDTH/2));
      __asm__("stx %v", colh);
      __asm__("lda #>(%v+512+256)", buf_0);
      __asm__("sta %g+2", cb0h_off0a);
      __asm__("sta %g+2", cb0h_off0b);
      __asm__("sta %g+2", cb0h_off1a);
      __asm__("sta %g+2", cb0h_off1b);
      __asm__("sta %g+2", cb0h_off1c);
      __asm__("sta %g+2", cb0h_off1d);
      __asm__("sta %g+2", cb0h_off2a);
      __asm__("sta %g+2", cb0h_off2b);
      __asm__("lda #>(%v+256)", buf_0);
      __asm__("sta %g+2", cb0l_off0a);
      __asm__("sta %g+2", cb0l_off0b);
      __asm__("sta %g+2", cb0l_off1a);
      __asm__("sta %g+2", cb0l_off1b);
      __asm__("sta %g+2", cb0l_off1c);
      __asm__("sta %g+2", cb0l_off1d);
      __asm__("sta %g+2", cb0l_off2a);
      __asm__("sta %g+2", cb0l_off2b);

      __asm__("lda #>(%v+512+256)", buf_1);
      __asm__("sta %g+2", cb1h_off0a);
      __asm__("sta %g+2", cb1h_off0b);
      __asm__("sta %g+2", cb1h_off0c);
      __asm__("sta %g+2", cb1h_off0d);
      __asm__("sta %g+2", cb1h_off0e);
      __asm__("sta %g+2", cb1h_off0f);
      __asm__("sta %g+2", cb1h_off0g);
      __asm__("sta %g+2", cb1h_off1a);
      __asm__("sta %g+2", cb1h_off1b);
      __asm__("sta %g+2", cb1h_off1c);
      __asm__("sta %g+2", cb1h_off1d);
      __asm__("sta %g+2", cb1h_off1e);
      __asm__("sta %g+2", cb1h_off1f);
      __asm__("sta %g+2", cb1h_off1g);
      __asm__("sta %g+2", cb1h_off1h);
      __asm__("sta %g+2", cb1h_off1i);
      __asm__("sta %g+2", cb1h_off1j);
      __asm__("sta %g+2", cb1h_off1k);
      __asm__("sta %g+2", cb1h_off2a);
      __asm__("sta %g+2", cb1h_off2b);
      __asm__("sta %g+2", cb1h_off2c);
      __asm__("sta %g+2", cb1h_off2d);
      __asm__("lda #>(%v+256)", buf_1);
      __asm__("sta %g+2", cb1l_off0a);
      __asm__("sta %g+2", cb1l_off0b);
      __asm__("sta %g+2", cb1l_off0c);
      __asm__("sta %g+2", cb1l_off0d);
      __asm__("sta %g+2", cb1l_off0e);
      __asm__("sta %g+2", cb1l_off0f);
      __asm__("sta %g+2", cb1l_off0g);
      __asm__("sta %g+2", cb1l_off1a);
      __asm__("sta %g+2", cb1l_off1b);
      __asm__("sta %g+2", cb1l_off1c);
      __asm__("sta %g+2", cb1l_off1d);
      __asm__("sta %g+2", cb1l_off1e);
      __asm__("sta %g+2", cb1l_off1f);
      // __asm__("sta %g+2", cb1l_off1g);
      __asm__("sta %g+2", cb1l_off1h);
      __asm__("sta %g+2", cb1l_off1i);
      __asm__("sta %g+2", cb1l_off1j);
      __asm__("sta %g+2", cb1l_off1k);
      __asm__("sta %g+2", cb1l_off2a);
      __asm__("sta %g+2", cb1l_off2b);
      __asm__("sta %g+2", cb1l_off2c);
      __asm__("sta %g+2", cb1l_off2d);

      __asm__("lda #>(%v+512+256)", buf_2);
      __asm__("sta %g+2", cb2h_off0a);
      __asm__("sta %g+2", cb2h_off0b);
      __asm__("sta %g+2", cb2h_off0c);
      __asm__("sta %g+2", cb2h_off0d);
      __asm__("sta %g+2", cb2h_off0e);
      __asm__("sta %g+2", cb2h_off1a);
      __asm__("sta %g+2", cb2h_off1b);
      __asm__("sta %g+2", cb2h_off1c);
      __asm__("sta %g+2", cb2h_off1d);
      __asm__("sta %g+2", cb2h_off1e);
      __asm__("sta %g+2", cb2h_off1f);
      __asm__("sta %g+2", cb2h_off1g);
      __asm__("sta %g+2", cb2h_off2b);
      __asm__("sta %g+2", cb2h_off2c);
      __asm__("lda #>(%v+256)", buf_2);
      __asm__("sta %g+2", cb2l_off0a);
      __asm__("sta %g+2", cb2l_off0b);
      __asm__("sta %g+2", cb2l_off0c);
      __asm__("sta %g+2", cb2l_off0d);
      __asm__("sta %g+2", cb2l_off0e);
      __asm__("sta %g+2", cb2l_off1a);
      __asm__("sta %g+2", cb2l_off1b);
      __asm__("sta %g+2", cb2l_off1c);
      __asm__("sta %g+2", cb2l_off1d);
      // __asm__("sta %g+2", cb2l_off1e);
      __asm__("sta %g+2", cb2l_off1f);
      __asm__("sta %g+2", cb2l_off1g);
      __asm__("sta %g+2", cb2l_off2b);
      __asm__("sta %g+2", cb2l_off2c);

      col_loop1:
        __asm__("lda %v", tree);
        __asm__("clc");
        __asm__("adc #>%v", huff_split);
        __asm__("sta %v", huff_num);
        __asm__("adc #1");
        __asm__("sta %v", huff_num_h);

        __asm__("jsr %v", getbithuff);
        __asm__("asl a");
        __asm__("sta %v", tree);

        __asm__("bne %g", tree_not_zero);
        __asm__("jmp %g", tree_zero);
        dechigh:
          __asm__("dec %v", colh);
          __asm__("dec %g+2", cb0h_off0a);
          __asm__("dec %g+2", cb0h_off0b);
          __asm__("dec %g+2", cb0h_off1a);
          __asm__("dec %g+2", cb0h_off1b);
          __asm__("dec %g+2", cb0h_off1c);
          __asm__("dec %g+2", cb0h_off1d);
          __asm__("dec %g+2", cb0h_off2a);
          __asm__("dec %g+2", cb0h_off2b);
          __asm__("dec %g+2", cb0l_off0a);
          __asm__("dec %g+2", cb0l_off0b);
          __asm__("dec %g+2", cb0l_off1a);
          __asm__("dec %g+2", cb0l_off1b);
          __asm__("dec %g+2", cb0l_off1c);
          __asm__("dec %g+2", cb0l_off1d);
          __asm__("dec %g+2", cb0l_off2a);
          __asm__("dec %g+2", cb0l_off2b);
          __asm__("dec %g+2", cb1h_off0a);
          __asm__("dec %g+2", cb1h_off0b);
          __asm__("dec %g+2", cb1h_off0c);
          __asm__("dec %g+2", cb1h_off0d);
          __asm__("dec %g+2", cb1h_off0e);
          __asm__("dec %g+2", cb1h_off0f);
          __asm__("dec %g+2", cb1h_off0g);
          __asm__("dec %g+2", cb1h_off1a);
          __asm__("dec %g+2", cb1h_off1b);
          __asm__("dec %g+2", cb1h_off1c);
          __asm__("dec %g+2", cb1h_off1d);
          __asm__("dec %g+2", cb1h_off1e);
          __asm__("dec %g+2", cb1h_off1f);
          __asm__("dec %g+2", cb1h_off1g);
          __asm__("dec %g+2", cb1h_off1h);
          __asm__("dec %g+2", cb1h_off1i);
          __asm__("dec %g+2", cb1h_off1j);
          __asm__("dec %g+2", cb1h_off1k);
          __asm__("dec %g+2", cb1h_off2a);
          __asm__("dec %g+2", cb1h_off2b);
          __asm__("dec %g+2", cb1h_off2c);
          __asm__("dec %g+2", cb1h_off2d);
          __asm__("dec %g+2", cb1l_off0a);
          __asm__("dec %g+2", cb1l_off0b);
          __asm__("dec %g+2", cb1l_off0c);
          __asm__("dec %g+2", cb1l_off0d);
          __asm__("dec %g+2", cb1l_off0e);
          __asm__("dec %g+2", cb1l_off0f);
          __asm__("dec %g+2", cb1l_off0g);
          __asm__("dec %g+2", cb1l_off1a);
          __asm__("dec %g+2", cb1l_off1b);
          __asm__("dec %g+2", cb1l_off1c);
          __asm__("dec %g+2", cb1l_off1d);
          __asm__("dec %g+2", cb1l_off1e);
          __asm__("dec %g+2", cb1l_off1f);
          // __asm__("dec %g+2", cb1l_off1g);
          __asm__("dec %g+2", cb1l_off1h);
          __asm__("dec %g+2", cb1l_off1i);
          __asm__("dec %g+2", cb1l_off1j);
          __asm__("dec %g+2", cb1l_off1k);
          __asm__("dec %g+2", cb1l_off2a);
          __asm__("dec %g+2", cb1l_off2b);
          __asm__("dec %g+2", cb1l_off2c);
          __asm__("dec %g+2", cb1l_off2d);
          __asm__("dec %g+2", cb2h_off0a);
          __asm__("dec %g+2", cb2h_off0b);
          __asm__("dec %g+2", cb2h_off0c);
          __asm__("dec %g+2", cb2h_off1a);
          __asm__("dec %g+2", cb2h_off1b);
          __asm__("dec %g+2", cb2h_off1c);
          __asm__("dec %g+2", cb2h_off1d);
          __asm__("dec %g+2", cb2h_off1e);
          __asm__("dec %g+2", cb2h_off2b);
          __asm__("dec %g+2", cb2h_off2c);
          __asm__("dec %g+2", cb2l_off0a);
          __asm__("dec %g+2", cb2l_off0b);
          __asm__("dec %g+2", cb2l_off0c);
          __asm__("dec %g+2", cb2l_off1a);
          __asm__("dec %g+2", cb2l_off1b);
          __asm__("dec %g+2", cb2l_off1c);
          __asm__("dec %g+2", cb2l_off1d);
          // __asm__("dec %g+2", cb2l_off1e);
          __asm__("dec %g+2", cb2l_off2b);
          __asm__("dec %g+2", cb2l_off2c);

          __asm__("dec %g+2", cb2l_off0d);
          __asm__("dec %g+2", cb2l_off0e);
          __asm__("dec %g+2", cb2h_off0d);
          __asm__("dec %g+2", cb2h_off0e);
          __asm__("dec %g+2", cb2l_off1f);
          __asm__("dec %g+2", cb2l_off1g);
          __asm__("dec %g+2", cb2h_off1f);
          __asm__("dec %g+2", cb2h_off1g);
          __asm__("jmp %g", declow);
        tree_not_zero:
          __asm__("sec");
          __asm__("lda %v", col);
          __asm__("beq %g", dechigh);
          declow:
          __asm__("sbc #2");
          __asm__("sta %v", col);

          __asm__("lda %v", tree);
          __asm__("cmp #16");
          __asm__("bne %g", tree_not_eight);

          // tree == 8
          __asm__("jsr %v", getbithuff36);
          __asm__("ldx %v", t);
          __asm__("jsr mult8x8r16_direct");
          __asm__("ldy %v", col);
cb1l_off1a:__asm__("sta $FF01,y");
          __asm__("txa");
cb1h_off1a:__asm__("sta $FF01,y");

          __asm__("jsr %v", getbithuff36);
          __asm__("ldx %v", t);
          __asm__("jsr mult8x8r16_direct");
          __asm__("ldy %v", col);
cb1l_off0a:__asm__("sta $FF00,y");
          __asm__("txa");
cb1h_off0a:__asm__("sta $FF00,y");

          __asm__("jsr %v", getbithuff36);
          __asm__("ldx %v", t);
          __asm__("jsr mult8x8r16_direct");
          __asm__("ldy %v", col);
cb2l_off1a:__asm__("sta $FF01,y");
          __asm__("txa");
cb2h_off1a:__asm__("sta $FF01,y");

          __asm__("jsr %v", getbithuff36);
          __asm__("ldx %v", t);
          __asm__("jsr mult8x8r16_direct");
          __asm__("ldy %v", col);
cb2l_off0a:__asm__("sta $FF00,y");
          __asm__("txa");
cb2h_off0a:__asm__("sta $FF00,y");

          __asm__("jmp %g", tree_done);

          tree_not_eight:
            //huff_ptr = huff[tree + 10];
            __asm__("adc #>(%v+10*256*2)", huff_split);
            __asm__("sta %v", huff_num);
            __asm__("adc #1");
            __asm__("sta %v", huff_num_h);

            // Get the four tk vals in advance
            //a
            __asm__("jsr %v", getbithuff);
            __asm__("tax");
            __asm__("bpl %g", pos1);
            __asm__("lda %v-128,x", shiftl4n_l);
            __asm__("sta %g+1", tk1_l);
            __asm__("lda %v-128,x", shiftl4n_h);
            __asm__("jmp %g", finish_bh1);
            pos1:
            __asm__("lda %v,x", shiftl4p_l);
            __asm__("sta %g+1", tk1_l);
            __asm__("lda %v,x", shiftl4p_h);

finish_bh1:
            __asm__("sta %g+1", tk1_h);

            __asm__("jsr %v", getbithuff);
            __asm__("tax");
            __asm__("bpl %g", pos2);
            __asm__("lda %v-128,x", shiftl4n_l);
            __asm__("sta %g+1", tk2_l);
            __asm__("lda %v-128,x", shiftl4n_h);
            __asm__("jmp %g", finish_bh2);
            pos2:
            __asm__("lda %v,x", shiftl4p_l);
            __asm__("sta %g+1", tk2_l);
            __asm__("lda %v,x", shiftl4p_h);

finish_bh2:
            __asm__("sta %g+1", tk2_h);

            __asm__("jsr %v", getbithuff);
            __asm__("tax");
            __asm__("bpl %g", pos3);
            __asm__("lda %v-128,x", shiftl4n_l);
            __asm__("sta %g+1", tk3_l);
            __asm__("lda %v-128,x", shiftl4n_h);
            __asm__("jmp %g", finish_bh3);
            pos3:
            __asm__("lda %v,x", shiftl4p_l);
            __asm__("sta %g+1", tk3_l);
            __asm__("lda %v,x", shiftl4p_h);

finish_bh3:
            __asm__("sta %g+1", tk3_h);

            __asm__("jsr %v", getbithuff);
            __asm__("tax");
            __asm__("bpl %g", pos4);
            __asm__("lda %v-128,x", shiftl4n_l);
            __asm__("sta %g+1", tk4_l);
            __asm__("lda %v-128,x", shiftl4n_h);
            __asm__("jmp %g", finish_bh4);
            pos4:
            __asm__("lda %v,x", shiftl4p_l);
            __asm__("sta %g+1", tk4_l);
            __asm__("lda %v,x", shiftl4p_h);

finish_bh4:
            __asm__("sta %g+1", tk4_h);

            __asm__("clc");
            __asm__("ldy %v", col);
cb0l_off2a: __asm__("lda $FF02,y");
cb1l_off2a: __asm__("adc $FF02,y");
            __asm__("tax");
cb0h_off2a: __asm__("lda $FF02,y");
cb1h_off2a: __asm__("adc $FF02,y");
            __asm__("cmp #$80");
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            __asm__("clc");
cb0l_off1a: __asm__("adc $FF01,y");
            __asm__("tax");
            __asm__("lda tmp1");
cb0h_off1a: __asm__("adc $FF01,y");
            __asm__("cmp #$80");
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            __asm__("clc");
tk1_l:      __asm__("adc #$FF");
cb1l_off1b: __asm__("sta $FF01,y");
            __asm__("lda tmp1");
tk1_h:      __asm__("adc #$FF");
cb1h_off1b: __asm__("sta $FF01,y");

            /* Second with col - 1*/
            __asm__("clc");
cb0l_off1b: __asm__("lda $FF01,y");
cb1l_off1c: __asm__("adc $FF01,y");
            __asm__("tax");
cb0h_off1b: __asm__("lda $FF01,y");
cb1h_off1c: __asm__("adc $FF01,y");
            __asm__("cmp #$80");
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            __asm__("clc");
cb0l_off0a: __asm__("adc $FF00,y");
            __asm__("tax");
            __asm__("lda tmp1");
cb0h_off0a: __asm__("adc $FF00,y");
            __asm__("cmp #$80");
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            /* Store to cur_buf_x */
            __asm__("clc");
tk2_l:      __asm__("adc #$FF");
cb1l_off0b: __asm__("sta $FF00,y");

            __asm__("lda tmp1");
tk2_h:      __asm__("adc #$FF");
cb1h_off0b: __asm__("sta $FF00,y");

            //b
            __asm__("clc");
cb1l_off2b: __asm__("lda $FF02,y");
cb2l_off2b: __asm__("adc $FF02,y");
            __asm__("tax");
cb1h_off2b: __asm__("lda $FF02,y");
cb2h_off2b: __asm__("adc $FF02,y");
            __asm__("cmp #$80");
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            __asm__("clc");
cb1l_off1d: __asm__("adc $FF01,y");
            __asm__("tax");
            __asm__("lda tmp1");
cb1h_off1d: __asm__("adc $FF01,y");
            __asm__("cmp #$80");
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            __asm__("clc");
tk3_l:      __asm__("adc #$FF");
cb2l_off1b: __asm__("sta $FF01,y");

            __asm__("lda tmp1");
tk3_h:      __asm__("adc #$FF");
cb2h_off1b: __asm__("sta $FF01,y");

            /* Second with col - 1*/
            __asm__("clc");
cb1l_off1e: __asm__("lda $FF01,y");
cb2l_off1c: __asm__("adc $FF01,y");
            __asm__("tax");
cb1h_off1e: __asm__("lda $FF01,y");
cb2h_off1c: __asm__("adc $FF01,y");
            __asm__("cmp #$80");
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            __asm__("clc");
cb1l_off0c: __asm__("adc $FF00,y");
            __asm__("tax");
            __asm__("lda tmp1");
cb1h_off0c: __asm__("adc $FF00,y");
            __asm__("cmp #$80");
            __asm__("ror a");
            __asm__("sta tmp1");
            __asm__("txa");
            __asm__("ror a");

            /* Store to cur_buf_x */
            __asm__("clc");
tk4_l:      __asm__("adc #$FF");
cb2l_off0b: __asm__("sta $FF00,y");

            __asm__("lda tmp1");
tk4_h:      __asm__("adc #$FF");
cb2h_off0b: __asm__("sta $FF00,y");

            __asm__("jmp %g", tree_done);

        tree_zero:
          nine_reps_loop:
            __asm__("ldx %v", colh);
            __asm__("bne %g", col_gt1a);
            __asm__("lda %v", col);
            __asm__("cmp #3");
            __asm__("bcs %g", col_gt1a);
            __asm__("lda #1"); /* nreps */
            __asm__("jmp %g", check_nreps);
            col_gt1a:
            __asm__("ldx #>(%v+9*256*2)", huff_split);
            __asm__("stx %v", huff_num);
            __asm__("inx");
            __asm__("stx %v", huff_num_h);
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

            __asm__("ldx #>(%v+10*256*2)", huff_split);
            __asm__("stx %v", huff_num);
            __asm__("inx");
            __asm__("stx %v", huff_num_h);
            __asm__("lda %v", col);
            do_rep_loop:
              __asm__("sec");
              __asm__("bne %g", declow2);
              __asm__("dec %v", colh);
              __asm__("dec %g+2", cb0h_off0a);
              __asm__("dec %g+2", cb0h_off0b);
              __asm__("dec %g+2", cb0h_off1a);
              __asm__("dec %g+2", cb0h_off1b);
              __asm__("dec %g+2", cb0h_off1c);
              __asm__("dec %g+2", cb0h_off1d);
              __asm__("dec %g+2", cb0h_off2a);
              __asm__("dec %g+2", cb0h_off2b);
              __asm__("dec %g+2", cb0l_off0a);
              __asm__("dec %g+2", cb0l_off0b);
              __asm__("dec %g+2", cb0l_off1a);
              __asm__("dec %g+2", cb0l_off1b);
              __asm__("dec %g+2", cb0l_off1c);
              __asm__("dec %g+2", cb0l_off1d);
              __asm__("dec %g+2", cb0l_off2a);
              __asm__("dec %g+2", cb0l_off2b);
              __asm__("dec %g+2", cb1h_off0a);
              __asm__("dec %g+2", cb1h_off0b);
              __asm__("dec %g+2", cb1h_off0c);
              __asm__("dec %g+2", cb1h_off0d);
              __asm__("dec %g+2", cb1h_off0e);
              __asm__("dec %g+2", cb1h_off0f);
              __asm__("dec %g+2", cb1h_off0g);
              __asm__("dec %g+2", cb1h_off1a);
              __asm__("dec %g+2", cb1h_off1b);
              __asm__("dec %g+2", cb1h_off1c);
              __asm__("dec %g+2", cb1h_off1d);
              __asm__("dec %g+2", cb1h_off1e);
              __asm__("dec %g+2", cb1h_off1f);
              __asm__("dec %g+2", cb1h_off1g);
              __asm__("dec %g+2", cb1h_off1h);
              __asm__("dec %g+2", cb1h_off1i);
              __asm__("dec %g+2", cb1h_off1j);
              __asm__("dec %g+2", cb1h_off1k);
              __asm__("dec %g+2", cb1h_off2a);
              __asm__("dec %g+2", cb1h_off2b);
              __asm__("dec %g+2", cb1h_off2c);
              __asm__("dec %g+2", cb1h_off2d);
              __asm__("dec %g+2", cb1l_off0a);
              __asm__("dec %g+2", cb1l_off0b);
              __asm__("dec %g+2", cb1l_off0c);
              __asm__("dec %g+2", cb1l_off0d);
              __asm__("dec %g+2", cb1l_off0e);
              __asm__("dec %g+2", cb1l_off0f);
              __asm__("dec %g+2", cb1l_off0g);
              __asm__("dec %g+2", cb1l_off1a);
              __asm__("dec %g+2", cb1l_off1b);
              __asm__("dec %g+2", cb1l_off1c);
              __asm__("dec %g+2", cb1l_off1d);
              __asm__("dec %g+2", cb1l_off1e);
              __asm__("dec %g+2", cb1l_off1f);
              // __asm__("dec %g+2", cb1l_off1g);
              __asm__("dec %g+2", cb1l_off1h);
              __asm__("dec %g+2", cb1l_off1i);
              __asm__("dec %g+2", cb1l_off1j);
              __asm__("dec %g+2", cb1l_off1k);
              __asm__("dec %g+2", cb1l_off2a);
              __asm__("dec %g+2", cb1l_off2b);
              __asm__("dec %g+2", cb1l_off2c);
              __asm__("dec %g+2", cb1l_off2d);
              __asm__("dec %g+2", cb2h_off0a);
              __asm__("dec %g+2", cb2h_off0b);
              __asm__("dec %g+2", cb2h_off0c);
              __asm__("dec %g+2", cb2h_off1a);
              __asm__("dec %g+2", cb2h_off1b);
              __asm__("dec %g+2", cb2h_off1c);
              __asm__("dec %g+2", cb2h_off1d);
              __asm__("dec %g+2", cb2h_off1e);
              __asm__("dec %g+2", cb2h_off2b);
              __asm__("dec %g+2", cb2h_off2c);
              __asm__("dec %g+2", cb2l_off0a);
              __asm__("dec %g+2", cb2l_off0b);
              __asm__("dec %g+2", cb2l_off0c);
              __asm__("dec %g+2", cb2l_off1a);
              __asm__("dec %g+2", cb2l_off1b);
              __asm__("dec %g+2", cb2l_off1c);
              __asm__("dec %g+2", cb2l_off1d);
              // __asm__("dec %g+2", cb2l_off1e);
              __asm__("dec %g+2", cb2l_off2b);
              __asm__("dec %g+2", cb2l_off2c);

              __asm__("dec %g+2", cb2l_off0d);
              __asm__("dec %g+2", cb2l_off0e);
              __asm__("dec %g+2", cb2h_off0d);
              __asm__("dec %g+2", cb2h_off0e);
              __asm__("dec %g+2", cb2l_off1f);
              __asm__("dec %g+2", cb2l_off1g);
              __asm__("dec %g+2", cb2h_off1f);
              __asm__("dec %g+2", cb2h_off1g);
              declow2:
              __asm__("sbc #2");
              __asm__("sta %v", col);
              __asm__("tay");

              __asm__("clc");
cb0l_off2b:   __asm__("lda $FF02,y");
cb1l_off2c:   __asm__("adc $FF02,y");
              __asm__("tax");
cb0h_off2b:   __asm__("lda $FF02,y");
cb1h_off2c:   __asm__("adc $FF02,y");
              __asm__("cmp #$80");
              __asm__("ror a");
              __asm__("sta tmp1");
              __asm__("txa");
              __asm__("ror a");

              __asm__("clc");
cb0l_off1c:   __asm__("adc $FF01,y");
              __asm__("tax");
              __asm__("lda tmp1");
cb0h_off1c:   __asm__("adc $FF01,y");
              __asm__("cmp #$80");
              __asm__("ror a");
cb1h_off1f:   __asm__("sta $FF01,y");
              __asm__("txa");
              __asm__("ror a");
cb1l_off1f:   __asm__("sta $FF01,y");

              /* Second */
              __asm__("clc");
// cb1l_off1g:   __asm__("lda $FF01,y"); already good
cb0l_off1d:   __asm__("adc $FF01,y");
              __asm__("tax");
cb1h_off1g:   __asm__("lda $FF01,y");
cb0h_off1d:   __asm__("adc $FF01,y");
              __asm__("cmp #$80");
              __asm__("ror a");
              __asm__("sta tmp1");
              __asm__("txa");
              __asm__("ror a");

              __asm__("clc");
cb0l_off0b:   __asm__("adc $FF00,y");
              __asm__("tax");
              __asm__("lda tmp1");
cb0h_off0b:   __asm__("adc $FF00,y");
              __asm__("cmp #$80");
              __asm__("ror a");
cb1h_off0d:   __asm__("sta $FF00,y");
              __asm__("txa");
              __asm__("ror a");
cb1l_off0d:   __asm__("sta $FF00,y");

              //d
              __asm__("clc");
cb1l_off2d:   __asm__("lda $FF02,y");
cb2l_off2c:   __asm__("adc $FF02,y");
              __asm__("tax");
cb1h_off2d:   __asm__("lda $FF02,y");
cb2h_off2c:   __asm__("adc $FF02,y");
              __asm__("cmp #$80");
              __asm__("ror a");
              __asm__("sta tmp1");
              __asm__("txa");
              __asm__("ror a");

              __asm__("clc");
cb1l_off1h:   __asm__("adc $FF01,y");
              __asm__("tax");
              __asm__("lda tmp1");
cb1h_off1h:   __asm__("adc $FF01,y");
              __asm__("cmp #$80");
              __asm__("ror a");
cb2h_off1d:   __asm__("sta $FF01,y");
              __asm__("txa");
              __asm__("ror a");
cb2l_off1d:   __asm__("sta $FF01,y");

              /* Second */
              __asm__("clc");
// cb2l_off1e:   __asm__("lda $FF01,y");
cb1l_off1i:   __asm__("adc $FF01,y");
              __asm__("tax");
cb2h_off1e:   __asm__("lda $FF01,y");
cb1h_off1i:   __asm__("adc $FF01,y");
              __asm__("cmp #$80");
              __asm__("ror a");
              __asm__("sta tmp1");
              __asm__("txa");
              __asm__("ror a");

              __asm__("clc");
cb1l_off0e:   __asm__("adc $FF00,y");
              __asm__("tax");
              __asm__("lda tmp1");
cb1h_off0e:   __asm__("adc $FF00,y");
              __asm__("cmp #$80");
              __asm__("ror a");
cb2h_off0c:   __asm__("sta $FF00,y");
              __asm__("txa");
              __asm__("ror a");
cb2l_off0c:   __asm__("sta $FF00,y");

              __asm__("lda %v", rep);
              __asm__("and #1");
              __asm__("beq %g", rep_even);

              // tk = getbithuff(8) << 4;
              __asm__("jsr %v", getbithuff);
              __asm__("tax");
              __asm__("bpl %g", pos5);
              __asm__("lda %v-128,x", shiftl4n_h);
              __asm__("sta tmp2");
              __asm__("lda %v-128,x", shiftl4n_l);
              __asm__("jmp %g", finish_bh5);
              pos5:
              __asm__("lda %v,x", shiftl4p_h);
              __asm__("sta tmp2");
              __asm__("lda %v,x", shiftl4p_l);
finish_bh5:
              __asm__("tax");

              //e
              __asm__("clc");
cb1l_off0f:   __asm__("adc $FF00,y");
cb1l_off0g:   __asm__("sta $FF00,y");
              __asm__("lda tmp2");
cb1h_off0f:   __asm__("adc $FF00,y");
cb1h_off0g:   __asm__("sta $FF00,y");

              __asm__("txa");
cb1l_off1j:   __asm__("adc $FF01,y");
cb1l_off1k:   __asm__("sta $FF01,y");
              __asm__("lda tmp2");
cb1h_off1j:   __asm__("adc $FF01,y");
cb1h_off1k:   __asm__("sta $FF01,y");

              __asm__("txa");
cb2l_off0d:   __asm__("adc $FF00,y");
cb2l_off0e:   __asm__("sta $FF00,y");
              __asm__("lda tmp2");
cb2h_off0d:   __asm__("adc $FF00,y");
cb2h_off0e:   __asm__("sta $FF00,y");

              __asm__("txa");
cb2l_off1f:   __asm__("adc $FF01,y");
cb2l_off1g:   __asm__("sta $FF01,y");
              __asm__("lda tmp2");
cb2h_off1f:   __asm__("adc $FF01,y");
cb2h_off1g:   __asm__("sta $FF01,y");

            rep_even:
            __asm__("ldx %v", rep);
            __asm__("inx");
            __asm__("cpx %v", rep_loop);
            __asm__("beq %g", rep_loop_done);
            __asm__("stx %v", rep);
            __asm__("lda %v", col);
            __asm__("beq %g", check_high);
            __asm__("jmp %g", do_rep_loop);
            check_high:
            __asm__("ldx %v", colh);
            __asm__("beq %g", col_loop1_done);
            __asm__("tay"); // fix ZERO flag needed at do_rep_loop
            __asm__("jmp %g", do_rep_loop);
            rep_loop_done:
            __asm__("lda %v", nreps);
            __asm__("cmp #9");
            __asm__("bne %g", nine_reps_loop_done);
            __asm__("jmp %g", nine_reps_loop);
          nine_reps_loop_done:
        tree_done:
      __asm__("lda %v", col);
      __asm__("beq %g", check_high2);
      __asm__("jmp %g", col_loop1);
      check_high2:
      __asm__("ldx %v", colh);
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
      __asm__("sty %v", cur_buf_0l);
      __asm__("sty %v", cur_buf_0h);
      __asm__("lda #>(%v)", buf_1);
      __asm__("sta %v+1", cur_buf_0l);
      __asm__("clc");
      __asm__("adc #>%w", DATABUF_SIZE/2);
      __asm__("sta %v+1", cur_buf_0h);

      loop5:
        __asm__("lda #4");
        __asm__("sta %v", x);
        x_loop_outer:
        __asm__("ldy #<%b", 0);
        x_loop:
          __asm__("sty tmp1");
          __asm__("tya");
          __asm__("lsr a");
          __asm__("tay");
#if 0 // define for precise divisions
          __asm__("lda (%v),y", cur_buf_0h);
          __asm__("tax");
          __asm__("lda (%v),y", cur_buf_0l);
          __asm__("jsr pushax");
          __asm__("lda %v", t);
          __asm__("jsr tosudiva0");
          __asm__("ldy tmp1");
#else
          __asm__("lda (%v),y", cur_buf_0h);
          __asm__("tax");
          __asm__("lda (%v),y", cur_buf_0l);
          __asm__("ldy %v", t);
          __asm__("jsr approx_div16x8_direct");
          __asm__("ldy tmp1");
#endif
          __asm__("cpx #0");
          __asm__("beq %g", no_val_clamp);
          __asm__("lda #$FF");
          __asm__("cpx #$80");
          __asm__("adc #$0");

          no_val_clamp:
          __asm__("sta (%v),y", raw_ptr1);

        __asm__("iny");
        __asm__("iny");
        __asm__("cpy #<%b", (WIDTH/4));
        __asm__("bne %g", x_loop);

        __asm__("clc");
        __asm__("lda %v", cur_buf_0l);
        __asm__("adc #<%w", (WIDTH/8));
        __asm__("sta %v", cur_buf_0l);
        __asm__("sta %v", cur_buf_0h);
        __asm__("lda %v+1", cur_buf_0l);
        __asm__("adc #>%w", (WIDTH/8));
        __asm__("sta %v+1", cur_buf_0l);
        __asm__("adc #>%w", DATABUF_SIZE/2);
        __asm__("sta %v+1", cur_buf_0h);

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
      __asm__("sty %v", cur_buf_0l);
      __asm__("sty %v", cur_buf_0h);
      __asm__("lda #>(%v)", buf_2);
      __asm__("sta %v+1", cur_buf_0l);
      __asm__("clc");
      __asm__("adc #>%w", DATABUF_SIZE/2);
      __asm__("sta %v+1", cur_buf_0h);

        __asm__("jmp %g", loop5);

      loop5_done:
      __asm__("clc");
      __asm__("ldx #>(%v+1)", buf_0); /* cur_buf[0]+1 */
      __asm__("lda #<(%v+1)", buf_0);
      __asm__("jsr pushax");

      __asm__("lda #<(%v)", buf_2); /* curbuf_2 */
      __asm__("ldx #>(%v)", buf_2);
      __asm__("jsr pushax");

      __asm__("lda #<%w", (USEFUL_DATABUF_SIZE-1));
      __asm__("ldx #>%w", (USEFUL_DATABUF_SIZE-1));
      __asm__("jsr _memcpy");

      __asm__("ldx #>(%v+512+1)", buf_0); /* cur_buf[0]+1 */
      __asm__("lda #<(%v+512+1)", buf_0);
      __asm__("jsr pushax");

      __asm__("lda #<(%v+512)", buf_2); /* curbuf_2 */
      __asm__("ldx #>(%v+512)", buf_2);
      __asm__("jsr pushax");

      __asm__("lda #<%w", (USEFUL_DATABUF_SIZE-1));
      __asm__("ldx #>%w", (USEFUL_DATABUF_SIZE-1));
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
      for (tree = 2, col = WIDTH/4; col; ) {
        huff_num = tree;
        tree = getbithuff()*2;
        if (tree) {
          col--;
          if (tree == 16) {
            getbithuff36();
            getbithuff36();
            getbithuff36();
            getbithuff36();
          } else {
            huff_num = (tree + 20);
            getbithuff();
            getbithuff();
            getbithuff();
            getbithuff();
          }
        } else {
          do {
            if (col > 1) {
              huff_num = 9*2;
              nreps = getbithuff() + 1;
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
                getbithuff();
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
      __asm__("lda #2");
      __asm__("sta %v", tree);
      __asm__("lda #<%w", (WIDTH/4));
      __asm__("sta %v", col);

      col_loop2:
        __asm__("lda %v", tree);
        __asm__("clc");
        __asm__("adc #>%v", huff_split);
        __asm__("sta %v", huff_num);
        __asm__("adc #1");
        __asm__("sta %v", huff_num_h);

        __asm__("jsr %v", getbithuff);
        __asm__("asl a");
        __asm__("sta %v", tree);

        __asm__("beq %g", tree_zero_2);
        tree_not_zero_2:
          __asm__("dec %v", col);

          //huff_ptr = huff[tree + 10];
          __asm__("cmp #16");
          __asm__("bne %g", norm_huff);
          __asm__("jsr %v", getbithuff36);
          __asm__("jsr %v", getbithuff36);
          __asm__("jsr %v", getbithuff36);
          __asm__("jsr %v", getbithuff36);
          __asm__("jmp %g", tree_zero_2_done);

          norm_huff:
          __asm__("clc");
          __asm__("adc #>(%v+10*256*2)", huff_split);
          __asm__("sta %v", huff_num);
          __asm__("adc #1");
          __asm__("sta %v", huff_num_h);

          __asm__("jsr %v", getbithuff);
          __asm__("jsr %v", getbithuff);
          __asm__("jsr %v", getbithuff);
          __asm__("jsr %v", getbithuff);

        __asm__("jmp %g", tree_zero_2_done);
        tree_zero_2:
            __asm__("lda %v", col);
            __asm__("cmp #2");
            __asm__("bcs %g", col_gt1);

            __asm__("lda #1"); /* nreps */
            __asm__("jmp %g", check_nreps_2);

            col_gt1:
            __asm__("ldx #>(%v+9*256*2)", huff_split);
            __asm__("stx %v", huff_num);
            __asm__("inx");
            __asm__("stx %v", huff_num_h);
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

            __asm__("ldx #>(%v+10*256*2)", huff_split);
            __asm__("stx %v", huff_num);
            __asm__("inx");
            __asm__("stx %v", huff_num_h);

            __asm__("ldx #$00");
            __asm__("stx %v", rep);
            do_rep_loop_2:
            __asm__("dec %v", col);

            __asm__("txa");
            __asm__("and #1");
            __asm__("beq %g", rep_even_2);

            __asm__("jsr %v", getbithuff);

            rep_even_2:
            __asm__("ldx %v", rep);
            __asm__("inx");
            __asm__("cpx %v", rep_loop);
            __asm__("beq %g", rep_loop_2_done);
            __asm__("stx %v", rep);
            __asm__("lda %v", col);
            __asm__("bne %g", do_rep_loop_2);
            __asm__("beq %g", check_c_loop);
            rep_loop_2_done:
          __asm__("lda %v", nreps);
          __asm__("cmp #9");
          __asm__("beq %g", tree_zero_2);
        tree_zero_2_done:
      __asm__("lda %v", col);
      __asm__("bne %g", col_loop2);
    check_c_loop:
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
    __asm__("lda %v", y);
    __asm__("and #1");
    __asm__("beq %g", even_y);
    __asm__("lda #<%b", (WIDTH/4)-2);
    __asm__("sta %g+1", end_copy_loop);
    __asm__("ldy #0");
    __asm__("sty %g+1", start_copy_loop);
    __asm__("jmp %g", copy_loop);
    even_y:
    __asm__("lda #<%b", (WIDTH/4)-1);
    __asm__("sta %g+1", end_copy_loop);
    __asm__("ldy #1");
    __asm__("sty %g+1", start_copy_loop);
    start_copy_loop:
    __asm__("ldy #$F0");
    copy_loop:
    __asm__("lda (%v),y", raw_ptr1);
    __asm__("iny");
    __asm__("sta (%v),y", raw_ptr1);
    __asm__("iny");
    end_copy_loop:
    __asm__("cpy #$F1");
    __asm__("bne %g", copy_loop);

    __asm__("clc");
    __asm__("lda %v", raw_ptr1);
    __asm__("adc #<%w", (WIDTH/4));
    __asm__("sta %v", raw_ptr1);
    __asm__("lda %v+1", raw_ptr1);
    __asm__("adc #>%w", (WIDTH/4));
    __asm__("sta %v+1", raw_ptr1);

    __asm__("dex");
    __asm__("bne %g", start_copy_loop);

    /* Finish Y loop */
    __asm__("dec %v", y);
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
#else
    initbithuff();
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
    if (!(row & 7)) {
      progress_bar(-1, -1, 80*22, (top + row), height);
    }
    #ifdef __CC65__
    huff_num = 0;
    __asm__("jsr %v", getbits6);
    __asm__("sta %v", t);
    __asm__("jsr %v", getbits6);
    __asm__("jsr %v", getbits6);
    #else
    huff_num = 255;
    t = getbits6();
    /* Ignore those */
    getbits6();
    getbits6();
    #endif

    c = 0;

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
