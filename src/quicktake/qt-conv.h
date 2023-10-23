#ifndef __qt_conv_h
#define __qt_conv_h

#include "platform.h"
#include "extended_conio.h"

#define QT_BAND 20
#define QT100_MAGIC "qktk"
#define QT150_MAGIC "qktn"

extern uint16 height, width, raw_width;
extern uint8 raw_image[];
extern uint16 raw_image_size;

extern char magic[5];
extern char *model;

extern uint32 bitbuf;
extern uint8 vbits;

extern FILE *ifp, *ofp;

uint8 get1();
uint16 get2();
uint8 getbithuff (uint8 nbits, uint16 *huff);
uint8 getbitnohuff (uint8 nbits);

void qt_load_raw(uint16 top, uint8 h);

#define RAW(row,col) raw_image[(row)*raw_width+(col)]
#define RAW_IDX(row,col) ((row)*raw_width+(col))
#define RAW_DIRECT_IDX(idx) raw_image[idx]

#define getbits(n) getbitnohuff(n)
#define FORC(cnt) for (c=0; c < cnt; c++)
#define FORC3 FORC(3)
#define FORC4 FORC(4)
#define FORCC FORC(COLORS)

#ifdef __CC65__

#define FAST_SHIFT_LEFT_8_LONG(value) do { \
    __asm__("lda %v+2", value);    \
    __asm__("sta %v+3", value);    \
    __asm__("lda %v+1", value);    \
    __asm__("sta %v+2", value);    \
    __asm__("lda %v", value);      \
    __asm__("sta %v+1", value);    \
    __asm__("stz %v", value);      \
} while (0)
#define FAST_SHIFT_LEFT_16_LONG(value) do {\
    __asm__("lda %v+1", value);    \
    __asm__("sta %v+3", value);    \
    __asm__("lda %v", value);      \
    __asm__("sta %v+2", value);    \
    __asm__("stz %v+1", value);    \
    __asm__("stz %v", value);      \
} while(0)
#define FAST_SHIFT_LEFT_24_LONG(value) do {\
    __asm__("lda %v", value);      \
    __asm__("sta %v+3", value);    \
    __asm__("stz %v+2", value);    \
    __asm__("stz %v+1", value);    \
    __asm__("stz %v", value);      \
} while(0)
#else
#define FAST_SHIFT_LEFT_8_LONG(value) (value <<=8)
#define FAST_SHIFT_LEFT_16_LONG(value) (value <<=16)
#define FAST_SHIFT_LEFT_24_LONG(value) (value <<=24)
#endif

#endif
