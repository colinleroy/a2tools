#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "qtk_bithuff.h"
#include "qt-conv.h"

static uint16 tmp;
static uint8 shift;
extern uint8 shiftl3[32];

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

uint8 __fastcall__ getbits6 (void) {
  uint8 r = 0;
  uint8 n = 6;
  while (n--) {
    r = (r<<1) | getbit();
  }
  // printf("has %8b\n", r);
  return r;
}

uint8 __fastcall__ getbithuff (void) {
  uint8 r = 0;
  uint8 n = 0;

  do {
    n++;
    // printf(" %8b not valid\n", r);
    r = (r<<1) | getbit();
  } while (huff_split[huff_num+1][r] != n);

  // printf("value for [%02d][%8b] = %d\n", huff_num, r, huff_split[huff_num][r]);
  return huff_split[huff_num][r];
}

uint8 __fastcall__ getbithuff36 (void) {
  uint8 r = 0;
  uint8 n = 5;
  while (n--) {
    r = (r<<1) | getbit();
  }
  return shiftl3[r];
}
