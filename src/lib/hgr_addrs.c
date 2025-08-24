
#include "platform.h"
#include "hgr.h"
#include "hgr_addrs.h"

#ifndef __CC65__
uint8 *hgr_baseaddr[HGR_HEIGHT];
uint8 hgr_baseaddr_l[HGR_HEIGHT];
uint8 hgr_baseaddr_h[HGR_HEIGHT];
uint8 div7_table[256];
uint8 mod7_table[256];
#endif

void init_hgr_base_addrs (void)
{
  static uint8 y, base_init_done = 0;
#ifndef __CC65__
  static uint16 x;
  uint8 *a, *b, line_of_eight, group_of_eight, group_of_sixtyfour;
#endif
  if (base_init_done) {
    return;
  }

  /* Fun with HGR memory layout! */
#ifndef __CC65__
  for (y = 0; y < HGR_HEIGHT; ++y)
  {
    line_of_eight = y % 8;
    group_of_sixtyfour = y / 64;
    group_of_eight = (y % 64) / 8;
    hgr_baseaddr[y] = (uint8 *)HGR_PAGE + line_of_eight * 1024 + group_of_eight * 128 + group_of_sixtyfour * 40;
  }

  a = div7_table + 0;

  /* Watch out - this is not a modulo 7 table going:
   * 0 1 2 3 4 5 6 0 1 2 3 4 5 6 ...
   * but a bitfield with the relevant bit set:
   * $01 $02 $04 $08 $10 ...
   * for ORing the current HGR byte
   */
  b = mod7_table + 0;

  for (x = 0; x < 256; x++) {
    *a = x / 7;
    *b = 1 << (x % 7);
    a++;
    b++;
  }
#else
  __asm__("ldy #0");
  __asm__("sty %v", y);

  __asm__("ldy #0");
  next_y:
    /* ABCDEFGH -> pppFGHCD EABAB000 */

    /* CD(E) */
    __asm__("tya");
    __asm__("and #$38");
    __asm__("lsr");
    __asm__("lsr");
    __asm__("lsr");
    __asm__("lsr");
    __asm__("sta tmp2");

    /* EABAB */
    __asm__("tya");
    __asm__("and #$C0");
    __asm__("ror"); /* E */
    __asm__("sta tmp1");
    __asm__("and #$60");
    __asm__("lsr");
    __asm__("lsr");
    __asm__("ora tmp1");
    __asm__("sta %v,y", hgr_baseaddr_l);

    /* FGH */
    __asm__("tya");
    __asm__("and #$7");
    __asm__("asl");
    __asm__("asl");
    __asm__("ora tmp2");

    /* ppp */
    __asm__("ora #$20"); /* first page */
    __asm__("sta %v,y", hgr_baseaddr_h);
  //}
  __asm__("iny");
  __asm__("cpy #%b", HGR_HEIGHT);
  __asm__("bne %g", next_y);
#endif

  base_init_done = 1;
}
