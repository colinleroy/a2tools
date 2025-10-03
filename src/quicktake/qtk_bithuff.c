#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "qtk_bithuff.h"
#include "qt-conv.h"

static uint16 tmp;
static uint8 shift;

int16 next_line[DATABUF_SIZE];
uint8 huff_split[18*2][256];
uint8 huff_ctrl[9*2][256];

uint8 huff_small_1[2] = {2, 254};
uint8 huff_small_2[2] = {253, 3};
uint8 huff_small_3[4] = {239, 251, 5, 17};
uint8 huff_small_4[4] = {249, 2, 9, 18};
uint8 huff_small_5[4] = {238, 247, 254, 7};

uint8 huff_data[4][256];
uint8 div48_l[256];
uint8 div48_h[256];
uint8 dyndiv_l[256];
uint8 dyndiv_h[256];

uint8 cache[CACHE_SIZE];
uint8 bitbuf=0;
uint8 vbits=0;

void initbithuff(void) {
  /* Consider we won't run out of cache there (at the very start). */
    bitbuf = *(cur_cache_ptr++);
    vbits = 8;
}

void refill(void) {
  bitbuf = *(cur_cache_ptr++);

  vbits = 8;
  if (cur_cache_ptr == cache_end) {
    read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
  }
}

uint8 getbit(void) {
  uint8 r;
  if (vbits == 0) {
    refill();
  }
  r = bitbuf & 0x80 ? 1:0;

  bitbuf <<= 1;
  vbits--;

  return r;
}

uint8 __fastcall__ getfactor (void) {
  uint8 r = 0;
  uint8 n = 6;
  while (n--) {
    r = (r<<1) | getbit();
  }
  // printf("has %8b\n", r);
  return r;
}

uint8 __fastcall__ getctrlhuff (uint8 huff_num) {
  uint8 r = 0;
  uint8 n = 0;

  do {
    n++;
    // printf(" %8b not valid\n", r);
    r = (r<<1) | getbit();
  } while (huff_ctrl[huff_num][r] != n);

  // printf("value for [%02d][%8b] = %d\n", huff_num, r, huff_split[huff_num][r]);
  return huff_ctrl[huff_num+1][r];
}

uint8 __fastcall__ getdatahuff_rep_val (void) {
  /* 0 => 0,
     10 => 2,
     11 => 254 */
  if (getbit() == 0) {
    return 0;
  }
  return getbit() ? 254 : 2;
}

static uint8 getdatahuff2(void) {
  return huff_small_2[getbit()];
}

static uint8 getdatahuff3(void) {
  uint8 r = (getbit()<<1) | getbit();
  return huff_small_3[r];
}

static uint8 getdatahuff4(void) {
  uint8 r = (getbit()<<1) | getbit();
  return huff_small_4[r];
}

static uint8 getdatahuff5(void) {
  uint8 r = (getbit()<<1) | getbit();
  return huff_small_5[r];
}

uint8 getdatahuff_nrepeats(void) {
  return getdatahuff_interpolate(0);
}
uint8 __fastcall__ getdatahuff_interpolate (uint8 huff_num) {
  uint8 r = 0;
  uint8 n = 0;

  switch (huff_num) {
    case 1: printf("THAT CANT BE\n"); return 0;
    case 2: return getdatahuff2();
    case 3: return getdatahuff3();
    case 4: return getdatahuff4();
    case 5: return getdatahuff5();
  }
  if (huff_num)
    huff_num -= 5;

  do {
    n++;
    // printf(" %8b not valid\n", r);
    r = (r<<1) | getbit();
  } while (huff_data[huff_num][r] != n);

  // printf("value for [%02d][%8b] = %d\n", huff_num, r, huff_split[huff_num][r]);
  return huff_data[huff_num][r+128];
}

uint8 __fastcall__ getdatahuff_init (void) {
  uint8 r = 0;
  uint8 n = 5;
  while (n--) {
    r = (r<<1) | getbit();
  }
  return (r<<3)|0x04;
}
