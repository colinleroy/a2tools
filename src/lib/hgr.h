#ifndef __hgr_h
#define __hgr_h

#include "platform.h"

#define HGR_LEN 8192U
#define HGR_WIDTH 280U
#define HGR_HEIGHT 192U

void __fastcall__ init_graphics(uint8 mono, uint8 dhgr);
void __fastcall__ init_text(void);
void __fastcall__ load_hgr_mono_file(unsigned char pages);
void __fastcall__ reserve_auxhgr_file(void);

extern const char hgr_auxfile[];

extern char hgr_mix_is_on;
extern char hgr_init_done;
extern char can_dhgr;

#ifdef __CC65__
#define HGR_PAGE  0x2000
#define HGR_PAGE2 0x4000

void __fastcall__ hgr_mixon(void);
void __fastcall__ hgr_mixoff(void);
#else
extern char HGR_PAGE[HGR_LEN];
#define hgr_mixon()
#define hgr_mixoff()
#define load_hgr_mono_file()
#endif

#endif
