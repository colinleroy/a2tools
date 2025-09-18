#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "qtk_bithuff.h"
#include "qt-conv.h"

static uint16 tmp;
static uint8 shift;
uint8 cache[CACHE_SIZE];
uint8 bitbuf=0, next = 0;
uint8 vbits=0;

void initbithuff(void) {
  /* Consider we won't run out of cache there (at the very start). */
    bitbuf = *(cur_cache_ptr++);
    next = *(cur_cache_ptr++);
    vbits = 8;
}

void refill(void) {
  bitbuf = next;
  next = *(cur_cache_ptr++);

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
  uint8 n;
  uint8 peek;
  int8 curvbits;
  /* weird special case, needing 8 bits but consuming 5.
   * forces us to to 16bit bitbuffer */
  n = 5;
  while (n--) {
    r = (r<<1) | getbit();
  }

  peek = bitbuf;
  curvbits = vbits;

  r = (r << 1) | (peek & 0x80 ? 1:0);
  peek <<= 1;
  curvbits--;
  if (curvbits == 0) {
    peek = next;
  }

  r = (r << 1) | (peek & 0x80 ? 1:0);
  peek <<= 1;
  curvbits--;
  if (curvbits == 0) {
    peek = next;
  }

  r = (r << 1) | (peek & 0x80 ? 1:0);
  // printf("huff36: value for %08b = %d\n", r, huff_split[huff_num][r]);
  return huff_split[36][r];
}
