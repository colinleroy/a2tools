/* Wait Time is from Call (JSR MON_WAIT) to End of Return to Caller (End of RTS);
 * Wait Time in Microseconds at the Non-Accelerated Speed of an Apple II Plus is:
 * [Time]=[Delay (5*(Acc*Acc)+27*Acc+26)/2 Cycles]*[1.023 Microseconds/Cycle]
 */

#include "platform.h"

void platform_sleep(uint8 s) {
  s *= 6;
  while(s > 0) {

    __asm__("bit $C082");
    __asm__("lda #$ff");  /* About 166ms */
    __asm__("jsr $fca8"); /* MONWAIT */
    __asm__("bit $C080");
    s--;
  }
}

/* CLOSE ENOUGH */
void platform_msleep(uint16 ms) {
  static uint16 lms;

  lms = ms;
  __asm__("bit $C082");
  __asm__("ldx %v", lms);
  __asm__("ldy %v+1", lms);
  __asm__("iny");
  __asm__("loop:");
    __asm__("lda #$11");
    __asm__("jsr $fca8");
    __asm__("dex");
    __asm__("bne loop");
    __asm__("dey");
    __asm__("bne loop");
  __asm__("bit $C080");
}
