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
  uint8 c;
  uint8 nbits = n;

  if (nbits == 0) {
    bitbuf = 0;
    vbits = 0;
    return 0;
  }
  if (vbits < 9) {
    initbithuff();
  }
  shift = 16-vbits;

  /* bitbuf is now 0xAABB and we want to get n bits from
   * the left. The following is decomposed in steps to make
   * it clearer what the asm implementation does with
   * shortcuts */

  tmp = (((uint32)(bitbuf)));
  /* tmp is now 0x0000AABB */

  tmp <<= shift;
  /* shift max 7, tmp is now 0x000AABB0 */

  tmp = (tmp & 0x0000FF00);
  /* tmp is now 0x0000AB00 */

  tmp >>= 8;
  /* tmp is now 0x000000AB */

  /* Final shift */
  shift = 8-nbits;
  c = (uint8)(tmp >> shift);

  if (huff_num != 255) {
    vbits -= huff_split[huff_num+1][c];
    return huff_split[huff_num][c];
  } else {
    vbits -= nbits;
    return c;
  }
}
