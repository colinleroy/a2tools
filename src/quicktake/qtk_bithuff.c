#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "qtk_bithuff.h"
#include "qt-conv.h"

static uint32 tmp;
static uint8 shift;
uint8 cache[CACHE_SIZE];
uint16 bitbuf=0;
uint8 vbits=0;

void initbithuff(void) {
  /* Refill as soon as possible
   * On the 6502 implementation, this
   * greatly simplifies shifting. */
  do {
    bitbuf <<= 8;
    if (cur_cache_ptr == cache_end) {
      read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
    }
    bitbuf |= *(cur_cache_ptr++);
    vbits += 8;
  } while (vbits < 9);
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
  if (vbits < 9) {
    initbithuff();
  } else {
    printf("got enough vbits (%d) to load %d\n", vbits, n);
  }
  shift = 16-vbits;
  printf("shift %d (%d vbits)\n", shift, vbits);
  /* bitbuf is now 0xAABB and we want to keep AA */
  tmp = (((uint32)(bitbuf << 8))) << shift;
  /* tmp is now 0x00AABB00 */
  tmp = (tmp & 0x00FF0000) >> 16;
  /* tmp is now 0xAA */

  shift = 8-nbits;
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
