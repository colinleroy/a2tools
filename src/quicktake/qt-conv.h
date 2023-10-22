#ifndef __qt_conv_h
#define __qt_conv_h

#include "platform.h"
#include "extended_conio.h"
#define QT_BAND 20

extern uint16 height, width, raw_width;
extern uint8 raw_image[(QT_BAND + 4) * 644];
extern char *magic;
extern char *model;

extern uint32 bitbuf;
extern uint8 vbits;

extern FILE *ifp, *ofp;

uint8 get1();
uint16 get2();
uint8 getbithuff (uint8 nbits, uint16 *huff);
void qt_load_raw(uint16 top, uint16 h);

#define RAW(row,col) raw_image[(row)*raw_width+(col)]
#define getbits(n) getbithuff(n,0)
#define FORC(cnt) for (c=0; c < cnt; c++)
#define FORC3 FORC(3)
#define FORC4 FORC(4)
#define FORCC FORC(COLORS)

#endif
