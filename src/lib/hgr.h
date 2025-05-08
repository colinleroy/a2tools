#ifndef __hgr_h
#define __hgr_h

#include "platform.h"

#define HGR_LEN 8192U
#define HGR_WIDTH 280U
#define HGR_HEIGHT 192U

void __fastcall__ init_hgr(uint8 mono);
void __fastcall__ init_text(void);
void __fastcall__ load_hgr_mono_file(unsigned char pages);

extern char hgr_mix_is_on;
extern char hgr_init_done;

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
