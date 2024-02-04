#include "hgr.h"

char hgr_init_done = 0;
char hgr_mix_is_on = 0;

#pragma code-name(push, "LOWCODE")

#ifndef __CC65__
#include "tgi_compat.h"

void init_hgr(uint8 mono) {
  tgi_init();
}
void init_text(void) {
  tgi_done();
}

#else

void init_hgr(uint8 mono) {
#ifdef __APPLE2__

#ifdef IIGS
  if (mono) {
    __asm__("lda     #$80");
    __asm__("sta     $C021"); /* MONOCOLOR */

    __asm__("lda     $C029"); /* NEWVIDEO */
    __asm__("ora     #$20");  /* Set bit 5 */
    __asm__("sta     $C029");

  } else {
    __asm__("lda     #$00");
    __asm__("sta     $C021"); /* MONOCOLOR */

    __asm__("lda     $C029"); /* NEWVIDEO */
    __asm__("and     #$DF");  /* Clear bit 5 */
    __asm__("sta     $C029");
  }
#endif

  #ifdef USE_HGR2
  __asm__("lda     #$40");
  #else
  __asm__("lda     #$20");
  #endif
  /* Set draw page */
  __asm__("sta     $E6"); /* HGRPAGE */

  /* Set graphics mode */
  __asm__("bit     $C000"); /* 80STORE */
  __asm__("bit     $C050"); /* TXTCLR */
  __asm__("bit     $C052"); /* MIXCLR */
  __asm__("bit     $C057"); /* HIRES */

  __asm__("bit $C00D");
  __asm__("bit $C00C");
  __asm__("bit $C05E");
  __asm__("bit $C05F");
  __asm__("bit $C05E");
  __asm__("bit $C05F");
  __asm__("bit $C00D");
  __asm__("bit $C05E");

  /* Set view page */
  #ifdef USE_HGR2
  __asm__("bit     $C055"); /* HISCR */
  #else
  __asm__("bit     $C054"); /* LOWSCR */
  #endif
#endif
  hgr_init_done = 1;
}

void init_text(void) {
#ifdef __APPLE2__
  __asm__("bit     $C054"); /* LOWSCR */
  __asm__("bit     $C051"); /* TXTSET */
  __asm__("bit     $C056"); /* LORES */
#endif
  hgr_init_done = 0;
}
#endif

#pragma code-name(pop)
