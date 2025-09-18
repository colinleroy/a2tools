#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "qtk_bithuff.h"
#include "qt-conv.h"

static uint16 tmp;
static uint8 shift;
uint8 cache[CACHE_SIZE];
uint16 bitbuf=0;
uint8 vbits=0;

extern uint8 huff_split[19*2][256]; /* codes in low valid in high */

void initbithuff(void) {
  /* Consider we won't run out of cache there (at the very start). */
    bitbuf = *(cur_cache_ptr++) << 8;
    bitbuf |= *(cur_cache_ptr++);
    vbits = 16;
}

void refillold(void) {
  /* Refill as soon as possible
   * On the 6502 implementation, this
   * greatly simplifies shifting. */
  bitbuf <<= 8;
  if (cur_cache_ptr == cache_end) {
    read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
  }
  bitbuf |= *(cur_cache_ptr++);
  vbits += 8;
}

void refill(void) {
  /* Refill as soon as possible
   * On the 6502 implementation, this
   * greatly simplifies shifting. */
  if (cur_cache_ptr == cache_end) {
    read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
  }
  bitbuf |= *(cur_cache_ptr++);
  vbits = 16;
}

uint8 getbit(void) {
  uint8 r = bitbuf & 0x8000 ? 1:0;

  bitbuf <<= 1;
  vbits--;
  if (vbits == 8) {
    refill();
  }

  return r;
}

uint8 __fastcall__ getbithuff (uint8 n) {
  uint8 r = 0;

  if (n == 6) {
    /* non-huff */
    while (n--) {
      r = (r<<1) | getbit();
    }
    // printf("has %8b\n", r);
    return r;
  }

  if (huff_num == 36) {
    /* weird special case, needing 8 bits but consuming 5.
     * forces us to to 16bit bitbuffer */
    n = 5;
    while (n--) {
      r = (r<<1) | getbit();
    }

    r = (r << 1) | (bitbuf & 0x8000 ? 1:0);
    r = (r << 1) | (bitbuf & 0x4000 ? 1:0);
    r = (r << 1) | (bitbuf & 0x2000 ? 1:0);
    return huff_split[huff_num][r];
  }
  
  n = huff_split[huff_num+1][0];
  // printf("bitbuf [%16b] huff %d, get %d bits, ", bitbuf, huff_num, n);
  if (n > 7) {
    // printf("(weird.)");
  }
  while (n--) {
    r = (r<<1) | getbit();
  }
  // printf("got %8b\n", r);

  n = huff_split[huff_num+1][0];
  while (huff_split[huff_num+1][r] != n) {
    n++;
    if (n > 8) {
      printf("no valid huff\n");
      exit(1);
    }
    printf(" %8b not valid\n", r);
    r = (r<<1) | getbit();
  }

  printf("value for [%02d][%8b] = %d\n", huff_num, r, huff_split[huff_num][r]);
  return huff_split[huff_num][r];
}

uint8 mask[] = {
  0xFF,
  0x7F,
  0x3F,
  0x1F,
  0x0F,
  0x07,
  0x03,
  0x01,
  0x00
};

static uint32 leftshifts = 0;
static uint32 rightshifts = 0;
uint8 __fastcall__ getbithuffold (uint8 n)
{
  uint8 c;
  uint8 nbits = n;
  uint8 rshift;
  uint16 tmp2;

  if (nbits == 0) {
    bitbuf = 0;
    vbits = 0;
    return 0;
  }
  if (vbits > 12) {
    goto shift_left_right;
  }
  if (vbits < 9) {
    refillold();
    goto shift_left_right;
  }

  // printf("bitbuf [%016b] want %d bits have %d ", bitbuf, nbits, vbits);
  if (1) {
    tmp = bitbuf;
    shift = vbits-nbits;
    tmp >>= shift;
    // printf("%d right b\n", shift);
    // rightshifts += shift;
    tmp &= mask[8-nbits];
    // printf(" shifted right %d [%016b] ", shift, (uint16)tmp);
    c = tmp;
  } else {
shift_left_right:
    tmp = bitbuf;
    shift = 16-vbits;
    // leftshifts += shift;
    tmp <<= shift;
    // printf("%d left\n", shift);
    /* shift max 7, tmp is now 0x000AABB0 */
    // printf(" shifted left %d [%016b] ", shift, (uint16)tmp);
    tmp >>= 8;
    /* Final shift */
    shift = 8-nbits;
    // printf("%d right a\n", shift);
    // rightshifts += shift;
    c = (uint8)(tmp >> shift);
  }

  // printf("shifts %d %d = %d\n", leftshifts, rightshifts, leftshifts+rightshifts);
  if (huff_num != 255) {
    uint8 r;
    // printf(" huff took %d bits, res [%08b] now bitbuf [%016b]\n", huff_split[huff_num+1][c], c, bitbuf);
    vbits -= huff_split[huff_num+1][c];
    r = c>> (8 - (huff_split[huff_num+1][c]));
    printf("value for [%02d][%8b] = %d\n", huff_num, r, huff_split[huff_num][c]);

    return huff_split[huff_num][c];
  } else {
    // printf(" std took %d bits, res [%08b] now bitbuf [%016b]\n", nbits, c, bitbuf);
    vbits -= nbits;
    return c;
  }
}
