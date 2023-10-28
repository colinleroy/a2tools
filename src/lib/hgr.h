#ifndef __hgr_h
#define __hgr_h

#define HGR_PAGE 0x2000
#define HGR_LEN 8192
#define HGR_WIDTH 280
#define HGR_HEIGHT 192

extern char hgr_init_done;
void init_hgr(void);
void init_text(void);

#endif
