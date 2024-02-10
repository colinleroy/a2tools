
#include "platform.h"
#include "hgr.h"
#include "hgr_addrs.h"

uint8 *hgr_baseaddr[HGR_HEIGHT];
uint8 div7_table[HGR_WIDTH];
uint8 mod7_table[HGR_WIDTH];

void init_hgr_base_addrs (void)
{
  static uint8 y, base_init_done = 0;
  uint16 x;
#ifndef __CC65__
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
  b = mod7_table + 0;

  for (x = 0; x < HGR_WIDTH; x++) {
    *a = x / 7;
    *b = 1 << (x % 7);
    a++;
    b++;
  }
#else
  __asm__("lda #<(%v)", hgr_baseaddr);
  __asm__("sta ptr2");
  __asm__("lda #>(%v)", hgr_baseaddr);
  __asm__("sta ptr2+1");
  __asm__("ldy #0");
  __asm__("sty %v", y);
  // for (y = 0; y < HGR_HEIGHT; ++y)
  // {
  next_y:
    /* use x as temporary variable */
    __asm__("lda #<%w", HGR_PAGE);
    __asm__("sta %v", x);
    __asm__("lda #>%w", HGR_PAGE);
    __asm__("sta %v+1", x);

    /* line of eight */
    __asm__("lda %v", y);
    __asm__("tax");
    __asm__("and #$07");
    __asm__("asl"); /* shift 2 and add to temp var high byte */
    __asm__("asl");
    __asm__("clc");
    __asm__("adc %v+1", x);
    __asm__("sta %v+1", x);

    /* group of sixty four */
    __asm__("stx ptr1"); /* Get Y back */
    __asm__("stz ptr1+1");
    __asm__("lda #64");
    __asm__("sta ptr4");
    __asm__("stz ptr4+1");
    __asm__("jsr udiv16");
    __asm__("lda ptr1"); /* quotient */
    __asm__("stz tmp1"); /* *4 */
    __asm__("asl");
    __asm__("rol tmp1");
    __asm__("asl");
    __asm__("rol tmp1");
    __asm__("ldx tmp1");
    __asm__("jsr mulax10"); /* and 10 */

    __asm__("clc");         /* and add to temp */
    __asm__("adc %v", x);
    __asm__("sta %v", x);
    __asm__("txa");
    __asm__("adc %v+1", x);
    __asm__("sta %v+1", x);

    /* group of eight */
    __asm__("lda sreg"); /* remainder, untouched by mulax10 */
    __asm__("lsr"); /* div by 8 */
    __asm__("lsr");
    __asm__("lsr");
    __asm__("ldx #0");
    __asm__("jsr aslax7");

    __asm__("clc");         /* and add to temp */
    __asm__("adc %v", x);
    __asm__("sta (ptr2)");
    __asm__("txa");
    __asm__("ldy #1");
    __asm__("adc %v+1", x);
    __asm__("sta (ptr2),y");

    __asm__("lda ptr2");
    __asm__("adc #2");
    __asm__("sta ptr2");
    __asm__("bcc %g", noof6);
    __asm__("inc ptr2+1");
    noof6:
    __asm__("clc");
    //hgr_baseaddr[y] = x; //(uint8 *)HGR_PAGE + line_of_eight * 1024 + group_of_eight * 128 + group_of_sixtyfour * 40;
  //}
  __asm__("inc %v", y);
  __asm__("lda %v", y);
  __asm__("cmp #<(%b)", HGR_HEIGHT);
  __asm__("bne %g", next_y);


  /* Precompute /7 and %7 from 0 to HGR_WIDTH */
  /* ptr2 and ptr3 are left alone by udiv16 */
  __asm__("lda #<(%v)", div7_table);
  __asm__("sta ptr2");
  __asm__("lda #>(%v)", div7_table);
  __asm__("sta ptr2+1");
  __asm__("lda #<(%v)", mod7_table);
  __asm__("sta ptr3");
  __asm__("lda #>(%v)", mod7_table);
  __asm__("sta ptr3+1");

  __asm__("lda #0");
  __asm__("sta %v", x);
  __asm__("sta %v+1", x);
  next_x:
    __asm__("sta ptr1");
    __asm__("lda %v+1", x);
    __asm__("sta ptr1+1");
    __asm__("lda #7");
    __asm__("sta ptr4");
    __asm__("stz ptr4+1");
    __asm__("jsr udiv16");
    __asm__("lda ptr1");
    __asm__("sta (ptr2)");
    __asm__("lda #1");
    __asm__("ldy sreg");
    __asm__("beq %g", no_shift);
    shift_b:
    __asm__("asl");
    __asm__("dey");
    __asm__("bne %g", shift_b);
    no_shift:
    __asm__("sta (ptr3)");

    __asm__("inc ptr2");
    __asm__("bne %g", noof7);
    __asm__("inc ptr2+1");
    noof7:

    __asm__("inc ptr3");
    __asm__("bne %g", noof8);
    __asm__("inc ptr3+1");
    noof8:
  __asm__("inc %v", x);
  __asm__("bne %g", noof9);
  __asm__("inc %v+1", x);
  noof9:
  __asm__("lda %v", x);
  __asm__("cmp #<(%w)", HGR_WIDTH);
  __asm__("bne %g", next_x);
  __asm__("ldx %v+1", x);
  __asm__("cpx #>(%w)", HGR_WIDTH);
  __asm__("bne %g", next_x);
#endif

  base_init_done = 1;
}
