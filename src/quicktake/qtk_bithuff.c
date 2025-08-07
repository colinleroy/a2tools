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

void initbithuff(void) {
  /* Refill as soon as possible
   * On the 6502 implementation, this
   * greatly simplifies shifting. */
  while (vbits <= 24) {
    bitbuf <<= 8;
    if (cur_cache_ptr == cache_end) {
      read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
    }
    bitbuf |= *(cur_cache_ptr++);
    vbits += 8;
  }
}
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
  if (vbits <= 24) {
    initbithuff();
  } else {
    printf("got enough vbits (%d) to load %d\n", vbits, n);
  }
  shift = 32-vbits;
  printf("shift %d (%d vbits)\n", shift, vbits);
  tmp = bitbuf << shift;

  tmp >>= 24;

  shift = 8-nbits; /* nbits max is 8 so we'll shift at least 24 */
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
