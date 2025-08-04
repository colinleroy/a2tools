#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "qtk_bithuff.h"
#include "qt-conv.h"

static uint32 tmp;
static uint8 shift;
uint8 cache[CACHE_SIZE];
uint32 bitbuf=0;
uint8 vbits=0;


uint8 __fastcall__ getbithuff (uint8 n)
{
  uint16 h;
  uint8 c;
  uint8 nbits = n;

  if (nbits == 0) {
    bitbuf = 0;
    vbits = 0;
    return 0;
  }
  if (vbits < nbits) {
    FAST_SHIFT_LEFT_8_LONG(bitbuf);
    if (cur_cache_ptr == cache_end) {
      read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
    }
    bitbuf |= *(cur_cache_ptr++);
    vbits += 8;
  }
  shift = 32-vbits;
  tmp = bitbuf << shift;
  if (shift >= 24) {
    FAST_SHIFT_LEFT_24_LONG_TO(bitbuf, tmp);
  } else if (shift >= 16) {
    FAST_SHIFT_LEFT_16_LONG_TO(bitbuf, tmp);
  } else if (shift >= 8) {
    FAST_SHIFT_LEFT_8_LONG_TO(bitbuf, tmp);
  }
  shift %= 8;
  if (shift)
    tmp <<= shift;

  shift = 8-nbits; /* nbits max is 8 so we'll shift at least 24 */
  FAST_SHIFT_RIGHT_24_LONG(tmp);
  if (shift)
    c = (uint8)(tmp >> shift);
  else
    c = (uint8)tmp;

  if (huff_ptr) {
    h = huff_ptr[c];
    vbits -= h >> 8;
    c = (uint8) h;
  } else {
    vbits -= nbits;
  }

  return c;
}
