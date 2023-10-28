#include "hgr.h"

char hgr_init_done = 0;

void init_hgr(void) {
#ifdef __APPLE2__

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
