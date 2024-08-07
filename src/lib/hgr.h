#ifndef __hgr_h
#define __hgr_h

#include "platform.h"

#define HGR_LEN 8192U
#define HGR_WIDTH 280U
#define HGR_HEIGHT 192U

#ifdef __CC65__
#define HGR_PAGE  0x2000
#define HGR_PAGE2 0x4000
#else
extern char HGR_PAGE[HGR_LEN];
#endif

extern char hgr_init_done;
extern char hgr_mix_is_on;
void init_hgr(uint8 mono);
void init_text(void);

#ifdef __CC65__
#define hgr_mixon() do { __asm__("bit $C053"); hgr_mix_is_on = 1; } while (0)
#define hgr_mixoff() do { __asm__("bit $C052"); hgr_mix_is_on = 0; } while (0)
#define switch_text() do {                \
  __asm__("bit     $C054"); /* LOWSCR */  \
  __asm__("bit     $C051"); /* TXTSET */  \
  __asm__("bit     $C056"); /* LORES */   \
} while (0)
#define hgr_mix_is_on() hgr_mix_is_on
#else
#define hgr_mixon()
#define hgr_mixoff()
#define hgr_mix_is_on() 0
#endif
#endif
