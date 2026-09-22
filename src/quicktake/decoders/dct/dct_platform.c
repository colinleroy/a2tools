#include <unistd.h>

#include "platform.h"
#include "dct_data.h"

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

void get_bitval(void) {
  bitval = valneg = 0;

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
    if (valneg) {
      bitval |= (bitmask_h[bitpos] << 8)|bitmask_l[bitpos] ;
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
