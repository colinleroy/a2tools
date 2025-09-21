#include "qtkn_platform.h"

static uint8 *raw_ptr1;
extern uint8 *row_idx, *row_idx_plus2;

void copy_data(void) {
    uint8 x, y;
#ifndef __CC65__
    raw_ptr1 = row_idx - 1;
    for (y = 4; y; y--) {
      int steps, i, j;
      x = (y & 1);
      if (x) {
        steps = (WIDTH/4)-2;
        x = 0;
      } else {
        steps = (WIDTH/4)-1;
        x = 1;
      }

      for (j = 4; j; j--) {
        for (i = x; i != steps; i+=2) {
          *(raw_ptr1 + (i+1)) = *(raw_ptr1+i);
        }
        raw_ptr1 += (WIDTH/4);
      }
    }

    row_idx += (WIDTH<<2);
    row_idx_plus2 += (WIDTH<<2);

#else
    //raw_ptr1 = row_idx - 1;
    __asm__("clc");
    __asm__("ldx %v+1", row_idx);
    __asm__("ldy %v", row_idx);
    __asm__("bne %g", nouf19);
    __asm__("dex");
    nouf19:
    __asm__("dey");
    __asm__("sty %v", raw_ptr1);
    __asm__("stx %v+1", raw_ptr1);

    __asm__("ldy #4");
    __asm__("sty %v", y);
    y_loop:

    __asm__("ldx #4");
    __asm__("lda %v", y);
    __asm__("and #1");
    __asm__("beq %g", even_y);
    __asm__("lda #<%b", (WIDTH/4)-2);
    __asm__("sta %g+1", end_copy_loop);
    __asm__("ldy #0");
    __asm__("sty %g+1", start_copy_loop);
    __asm__("jmp %g", copy_loop);
    even_y:
    __asm__("lda #<%b", (WIDTH/4)-1);
    __asm__("sta %g+1", end_copy_loop);
    __asm__("ldy #1");
    __asm__("sty %g+1", start_copy_loop);
    start_copy_loop:
    __asm__("ldy #$F0");
    copy_loop:
    __asm__("lda (%v),y", raw_ptr1);
    __asm__("iny");
    __asm__("sta (%v),y", raw_ptr1);
    __asm__("iny");
    end_copy_loop:
    __asm__("cpy #$F1");
    __asm__("bne %g", copy_loop);

    __asm__("clc");
    __asm__("lda %v", raw_ptr1);
    __asm__("adc #<%w", (WIDTH/4));
    __asm__("sta %v", raw_ptr1);
    __asm__("lda %v+1", raw_ptr1);
    __asm__("adc #>%w", (WIDTH/4));
    __asm__("sta %v+1", raw_ptr1);

    __asm__("dex");
    __asm__("bne %g", start_copy_loop);

    /* Finish Y loop */
    __asm__("dec %v", y);
    __asm__("bne %g", y_loop);

    __asm__("clc");
    __asm__("lda %v", row_idx);
    __asm__("adc #<%w", (WIDTH<<2));
    __asm__("sta %v", row_idx);
    __asm__("lda %v+1", row_idx);
    __asm__("adc #>%w", (WIDTH<<2));
    __asm__("sta %v+1", row_idx);

    __asm__("lda %v", row_idx_plus2);
    __asm__("adc #<%w", (WIDTH<<2));
    __asm__("sta %v", row_idx_plus2);
    __asm__("lda %v+1", row_idx_plus2);
    __asm__("adc #>%w", (WIDTH<<2));
    __asm__("sta %v+1", row_idx_plus2);
#endif
}
