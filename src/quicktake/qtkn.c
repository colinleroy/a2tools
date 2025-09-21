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
#include "qtkn_platform.h"
#include "approxdiv16x8.h"


/* Shared with qt-conv.c */
char magic[5] = QTKN_MAGIC;
char *model = "150";

extern uint8 cache[CACHE_SIZE];
uint8 *cache_start = cache;
uint8 raw_image[RAW_IMAGE_SIZE];

uint8 val_hi_from_last[17] = {
  0x00, 0x10, 0x08, 0x05, 0x04, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
};
uint8 val_from_last[256] = {
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
uint8 factor;
/* Wastes bytes, but simplifies adds */

#ifndef __CC65__
uint16 val;
static uint8 tmp8;
static uint16 tmp16;
static uint32 tmp32;
static uint8 shiftl4p_l[128];
static uint8 shiftl4p_h[128];
static uint8 shiftl4n_l[128];
static uint8 shiftl4n_h[128];
uint8 div48_l[256];
uint8 div48_h[256];
uint8 shiftl3[32];
#endif

#ifdef __CC65__
extern uint8 buf_0[DATABUF_SIZE];
extern uint8 buf_1[DATABUF_SIZE];
extern uint8 buf_2[DATABUF_SIZE];

extern uint8 shiftl4p_l[128];
extern uint8 shiftl4p_h[128];
extern uint8 shiftl4n_l[128];
extern uint8 shiftl4n_h[128];
extern uint8 shiftl3[32];
extern uint8 div48_l[256];
extern uint8 div48_h[256];

extern uint8 huff_ctrl[9*2][256];
extern uint8 huff_data[9][256];
extern uint8 huff_numc, huff_numc_h;
extern uint8 huff_numd, huff_numd_h;
extern void refill_ret, getctrlhuff_refilled, getdatahuff_refilled, getdatahuff8_refilled;

#define raw_ptr1 zp2ip
#define cur_buf_0l zp4p
#define cur_buf_0h zp6p
#define col zp7
static uint8 colh;
// zp8-12 used by bitbuffer
#define rep zp13

#else
uint8 buf_0[DATABUF_SIZE];
uint8 buf_1[DATABUF_SIZE];
uint8 buf_2[DATABUF_SIZE];
static uint16 col;
uint8 huff_ctrl[9*2][256];
uint8 huff_data[9][256];
uint8 huff_num;
uint8 rep;
static uint8 *raw_ptr1;

static uint8 *cur_buf_0l, *cur_buf_1l, *cur_buf_2l;
static uint8 *cur_buf_0h, *cur_buf_1h, *cur_buf_2h;

#endif

uint8 *row_idx, *row_idx_plus2;

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
uint8 last = 16;

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

static void init_top_b(void);
#pragma code-name(push, "LC")
void init_top(void) {
  static uint8 l, h;

  l = 0;
  h = 1;
  for (s = i = 0; l < 18; i += 2) {
    uint8 code = s & 0xFF;
    r = src[i];
    t = 256 >> r;

    code >>= 8-r;
    huff_ctrl[h][code] = src[i+1];
    huff_ctrl[l][code] = r;
    // printf("huff_ctrl[%d][%.*b] = %d (r%d)\n", l, r, code, src[i+1], r);

    if (s >> 8 != (s+t) >> 8) {
      l += 2;
      h += 2;
    }
    s += t;
  }

  l = 0;
  h = 1;
  for (; i != sizeof src; i += 2) {
    uint8 code = s & 0xFF;
    r = src[i];
    t = 256 >> r;

    code >>= 8-r;
    huff_data[l][code+128] = src[i+1];
    huff_data[l][code] = r;
    // printf("huff_data[%d][%.*b] = %d (r%d)\n", l, r, code, src[i+1], r);

    if (s >> 8 != (s+t) >> 8) {
      l++;
    }
    s += t;
  }

  for (c=0; c != 32; c++) {
    shiftl3[c] = (c<<3)+4;
    // printf("huff[%d][%.*b] = %d (r%d)\n", 36, 5, c, (c<<3)+4, 5);
  }
  r = 0;
  do {
    /* 48 is the most common multiplier and later divisor.
     * It is worth it to approximate those divisions HARD
     * by rounding the numerator to the nearest 256, in order
     * to have a size-appropriate table. */
    uint16 approx = (r<<8)/48;
    div48_l[r] = approx & 0xFF;
    div48_h[r] = approx >> 8;
    // printf("%d/48 = %d\n", r<<8, div48_l[r]+(div48_h[r]<<8));
  } while (++r);

  init_top_b();
}

#pragma code-name(pop)
static void init_top_b(void) {

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

#pragma code-name(push, "LC")
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

  row_idx = raw_image;
  row_idx_plus2 = raw_image + (WIDTH*2);

  for (row=0; row != BAND_HEIGHT; row+=4) {
    if (!(row & 7)) {
      progress_bar(-1, -1, 80*22, (top + row), height);
    }
    factor = getbits6();
    /* Ignore those */
    getbits6();
    getbits6();

    c = 0;

    init_row();

    decode_row();
    consume_extra();

    copy_data();
  }
}
#pragma code-name(pop)

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
