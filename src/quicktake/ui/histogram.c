#include "platform.h"

#ifndef __CC65__
int8 err_buf[512+2];
static uint16 histogram[256];
uint8 opt_histogram[256];
#else
extern int8 err_buf[512+2];
extern uint8 opt_histogram[256];
#endif

void histogram_autolevel(void) {
  uint16 curr_hist = 0;
  uint8 range, i;
  uint8 black = 0, found_black = 0;
  uint8 white = 0, found_white = 0;
  i = 0;
#ifndef __CC65__
  do {
    curr_hist += (uint16)(err_buf[i] | (err_buf[i+256] << 8));
    if (!found_black && curr_hist > 512) {
      found_black = 1;
      black = i;
    }
    if (!found_white && curr_hist > 99UL*256*192/100) {
      found_white = 1;
      white = i;
      range = white - black;
    }
    i++;
  } while (i);

  do {
    if (i <= black) {
      opt_histogram[i] = 0;
    } else if (i >= white) {
      opt_histogram[i] = 255;
    } else {
      opt_histogram[i] = ((i - black) * 255UL / range);
    }
    i++;
  } while (i);
#else
  __asm__("ldx #$00");
prepare_autolevel:
  __asm__("lda %v,x", err_buf);
  __asm__("adc %v", curr_hist);
  __asm__("sta %v", curr_hist);
  __asm__("lda %v+256,x", err_buf);
  __asm__("adc %v+1", curr_hist);
  __asm__("sta %v+1", curr_hist);

  __asm__("ldy %v", found_black);
  __asm__("bne %g", check_white);
  __asm__("cmp #>(256)");         /* ~ 1% */
  __asm__("bcc %g", check_white);
  /* We found our black value */
  __asm__("inc %v", found_black);
  __asm__("stx %v", black);
check_white:
  __asm__("ldy %v", found_white);
  __asm__("bne %g", cont_loop);
  __asm__("cmp #>(49152-256)");   /* ~99% */
  __asm__("bcc %g", cont_loop);
  __asm__("inc %v", found_white);
  __asm__("stx %v", white);
  __asm__("txa");
  __asm__("sec");
  __asm__("sbc %v", black);
  __asm__("sta %v", range);
cont_loop:
  __asm__("inx");
  __asm__("bne %g", prepare_autolevel);

  /* Now expand linearly */
  __asm__("lda #0");
compute_histogram:
  __asm__("cpx %v", black);
  __asm__("bcc %g", store);

  __asm__("cpx %v", white);
  __asm__("bcs %g", full_white);

  __asm__("stx tmp1");        /* Backup X */
  __asm__("txa");
  __asm__("sec");
  __asm__("sbc %v", black);
  __asm__("sta tmp2");
  __asm__("tax");             /* X = high byte (*256) */
  __asm__("lda #0");
  __asm__("sec");
  __asm__("sbc tmp2");        /* - i-black => *255 */
  __asm__("tay");
  __asm__("txa");
  __asm__("sbc #0");
  __asm__("tax");
  __asm__("tya");
  __asm__("jsr pushax");
  __asm__("lda %v", range);
  __asm__("jsr tosudiva0");
  __asm__("ldx tmp1");
  __asm__("jmp %g", store);
  
full_white:
  __asm__("lda #255");
store:
  __asm__("sta %v,x", opt_histogram);
  __asm__("inx");
  __asm__("bne %g", compute_histogram);
#endif
}

#pragma code-name(push, "SQUEEZE")

void histogram_contrast(void) {
  uint16 curr_hist = 0;
#ifndef __CC65__
  cur_opt_histogram = opt_histogram;
  cur_histogram = histogram;
  do {
    uint32 tmp;
    curr_hist += *(cur_histogram++);
    tmp = (curr_hist*255) / (256*192);
    *(cur_opt_histogram++) = tmp;
  } while (++x);
#else
  __asm__("ldx #0");
  next_h:
  __asm__("stx tmp1");
  __asm__("clc");
  __asm__("lda %v,x", err_buf);
  __asm__("adc %v", curr_hist);
  __asm__("sta %v", curr_hist);
  /* curr_hist*255 done as curr_hist*256 - curr_hist */
  __asm__("tay"); /* *256 mid-byte to Y */

  __asm__("lda %v+256,x", err_buf);
  __asm__("adc %v+1", curr_hist);
  __asm__("sta %v+1", curr_hist);
  __asm__("tax"); /* *256 high byte saved to X */

  __asm__("lda #0"); /* *256 low-byte = 0 */

  /* -curr_hist => curr_hist * 255 */
  __asm__("sec");
  __asm__("sbc %v", curr_hist); /* don't store low-byte, it'll be discarded on div 256 */
  __asm__("tya");
  __asm__("sbc %v+1", curr_hist);
  __asm__("tay");               /* mid-byte back to Y */
  __asm__("txa");
  __asm__("sbc #0");
  __asm__("tax");               /* high byte back to X */

  /* /(width*height) done as /width /height */
  /* / 256 */
  __asm__("tya");               /* move mid-byte to A. AX now correct for last div */

  /* / 192 */
  __asm__("jsr pushax");
  __asm__("lda #<%w", 192);
  __asm__("jsr tosudiva0");
  __asm__("ldx tmp1");

  __asm__("sta %v,x", opt_histogram);

  __asm__("inx");
  __asm__("bne %g", next_h);
#endif
}
