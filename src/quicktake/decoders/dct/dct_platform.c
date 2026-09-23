#include <unistd.h>
#include <assert.h>

#include "platform.h"
#include "dct_data.h"
#include "dct_platform.h"

int8 mul_362(int8 w)
{
  int16 x;
  x = (int16)(w * 362);
  x >>= 8;
  return (int8)x & 0xFF;
}

int8 mul_473(int8 w)
{
  int16 x;
  x = (int16)(w * 473);
  x >>= 8;
  return (int8)x & 0xFF;
}

int8 mul_277(int8 w)
{
  int16 x;
  x = (int16)(w * 277);
  x >>= 8;
  return (int8)x & 0xFF;
}

int8 mul_669(int8 w)
{
  int16 x;
  x = (int16)(w * 669);
  x >>= 8;
  return (int8)x & 0xFF;
}

int8 ign_bits;

void get_coeffs(void) {
  for (scan = 0; scan < 64; scan++) {
      uint8 r = SCAN[scan];

      if (!(numbits = bits_table[scan])) {
          coef[r] = 0;
          continue;
      }
      ign_bits = shift_table[scan];

      bitpos = 0;

      get_bitval();

      /* extend sign bit */
      if (scan && valneg) {
          bitval = (uint16)bitval | negate[bitpos];
      }
      coef[r] = (int8)(bitval);
  }
}

void get_bitval(void) {
  bitval = valneg = 0;

  assert(numbits-ign_bits < 8);
  do {
    if (--nbits_avail < 0) {
        nbits_avail = 7;
        cur_cache_ptr++;
        if (cur_cache_ptr == cache_end) {
            read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
        }
    }

    valneg = *cur_cache_ptr & 1;
    *cur_cache_ptr >>= 1;
    if (--ign_bits >= 0) {
      continue;
    }
    if (valneg) {
      bitval |= bitmask[bitpos] ;
    }
    bitpos++;
  } while (--numbits);
}

void advance_block(void) {
  if (--blocks_rem_in_row == 0) {
    if (actual_width == 160) {
      idx += 8*RAW_WIDTH-DECODE_WIDTH+16;
    } else {
      idx += 4*RAW_WIDTH-DECODE_WIDTH+8;
    }
    printf("idx %p\n", idx);
    blocks_rem_in_row = blocks_per_row;
  } else {
    idx += actual_width == 160 ? 16 : 8;
  }
}
