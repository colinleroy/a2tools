#ifndef __hgr_h
#define __hgr_h

#define HGR_LEN 8192
#define HGR_WIDTH 280
#define HGR_HEIGHT 192

#ifdef __CC65__
#define HGR_PAGE 0x2000
#else
extern char HGR_PAGE[HGR_LEN];
#endif

extern char hgr_init_done;
void init_hgr(void);
void init_text(void);

#endif
