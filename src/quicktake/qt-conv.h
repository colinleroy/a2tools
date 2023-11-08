#ifndef __qt_conv_h
#define __qt_conv_h

#include "platform.h"
#include "extended_conio.h"

#define QT_BAND 20

#define SCALE 1

#define QT100_MAGIC "qktk"
#define QT150_MAGIC "qktn"

#if SCALE
#define FILE_WIDTH 256
#define FILE_HEIGHT HGR_HEIGHT
#else
#define FILE_WIDTH width
#define FILE_HEIGHT height
#endif


#ifdef __CC65__
#define TMP_NAME "/RAM/GREY"
#define HIST_NAME "/RAM/HIST"
#else
#define TMP_NAME "GREY"
#define HIST_NAME "HIST"
#endif

extern uint16 height, width;
extern uint8 raw_image[];
extern uint16 raw_width, raw_image_size;

extern char magic[5];
extern char *model;

extern uint32 bitbuf;
extern uint8 vbits;

extern FILE *ifp, *ofp;

void iseek(uint32 off);
uint32 cache_read_since_inval(void);
extern uint8 cache[];
extern uint16 cache_size;

uint8 get1();
uint16 get2();
uint8 getbithuff (uint8 nbits, uint16 *huff);
uint8 getbitnohuff (uint8 nbits);

void qt_load_raw(uint16 top, uint8 h);

#define RAW(row,col) raw_image[((row)*width)+(col)]
#define RAW_IDX(row,col) (((row)*width)+(col))
#define RAW_DIRECT_IDX(idx) raw_image[idx]

#define FILE(row,col) raw_image[((row)*width)+(col)]
#define FILE_IDX(row,col) (((row)*width)+(col))
#define FILE_DIRECT_IDX(idx) raw_image[idx]

#define getbits(n) getbitnohuff(n)

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

#define FAST_SHIFT_RIGHT_8_LONG(value) do { \
    __asm__("lda %v+1", value);    \
    __asm__("sta %v", value);      \
    __asm__("lda %v+2", value);    \
    __asm__("sta %v+1", value);    \
    __asm__("lda %v+3", value);    \
    __asm__("sta %v+2", value);    \
    __asm__("stz %v+3", value);    \
} while(0)
#define FAST_SHIFT_RIGHT_16_LONG(value) do { \
    __asm__("lda %v+2", value);    \
    __asm__("sta %v", value);      \
    __asm__("lda %v+3", value);    \
    __asm__("sta %v+1", value);    \
    __asm__("stz %v+2", value);    \
    __asm__("stz %v+3", value);    \
} while(0)
#define FAST_SHIFT_RIGHT_24_LONG(value) do { \
    __asm__("lda %v+3", value);    \
    __asm__("sta %v", value);      \
    __asm__("stz %v+1", value);    \
    __asm__("stz %v+2", value);    \
    __asm__("stz %v+3", value);    \
} while(0)

#define FAST_SHIFT_LEFT_8_LONG_TO(value, to) do { \
    __asm__("lda %v+2", value);    \
    __asm__("sta %v+3", to);    \
    __asm__("lda %v+1", value);    \
    __asm__("sta %v+2", to);    \
    __asm__("lda %v", value);      \
    __asm__("sta %v+1", to);    \
    __asm__("stz %v", to);      \
} while (0)
#define FAST_SHIFT_LEFT_16_LONG_TO(value, to) do {\
    __asm__("lda %v+1", value);    \
    __asm__("sta %v+3", to);    \
    __asm__("lda %v", value);      \
    __asm__("sta %v+2", to);    \
    __asm__("stz %v+1", to);    \
    __asm__("stz %v", to);      \
} while(0)
#define FAST_SHIFT_LEFT_24_LONG_TO(value, to) do {\
    __asm__("lda %v", value);      \
    __asm__("sta %v+3", to);    \
    __asm__("stz %v+2", to);    \
    __asm__("stz %v+1", to);    \
    __asm__("stz %v", to);      \
} while(0)

#define FAST_SHIFT_RIGHT_8_LONG_TO(value, to) do { \
    __asm__("lda %v+1", value);    \
    __asm__("sta %v", to);      \
    __asm__("lda %v+2", value);    \
    __asm__("sta %v+1", to);    \
    __asm__("lda %v+3", value);    \
    __asm__("sta %v+2", to);    \
    __asm__("stz %v+3", to);    \
} while(0)
#define FAST_SHIFT_RIGHT_16_LONG_TO(value, to) do { \
    __asm__("lda %v+2", value);    \
    __asm__("sta %v", to);      \
    __asm__("lda %v+3", value);    \
    __asm__("sta %v+1", to);    \
    __asm__("stz %v+2", to);    \
    __asm__("stz %v+3", to);    \
} while(0)
#define FAST_SHIFT_RIGHT_24_LONG_TO(value, to) do { \
    __asm__("lda %v+3", value);    \
    __asm__("sta %v", to);      \
    __asm__("stz %v+1", to);    \
    __asm__("stz %v+2", to);    \
    __asm__("stz %v+3", to);    \
} while(0)

#else
#define FAST_SHIFT_LEFT_8_LONG(value) (value <<=8)
#define FAST_SHIFT_LEFT_16_LONG(value) (value <<=16)
#define FAST_SHIFT_LEFT_24_LONG(value) (value <<=24)
#define FAST_SHIFT_RIGHT_8_LONG(value) (value >>=8)
#define FAST_SHIFT_RIGHT_16_LONG(value) (value >>=16)
#define FAST_SHIFT_RIGHT_24_LONG(value) (value >>=24)

#define FAST_SHIFT_LEFT_8_LONG_TO(value, to) (to = value << 8)
#define FAST_SHIFT_LEFT_16_LONG_TO(value, to) (to = value << 16)
#define FAST_SHIFT_LEFT_24_LONG_TO(value, to) (to = value << 24)
#define FAST_SHIFT_RIGHT_8_LONG_TO(value, to) (to = value >> 8)
#define FAST_SHIFT_RIGHT_16_LONG_TO(value, to) (to = value >> 16)
#define FAST_SHIFT_RIGHT_24_LONG_TO(value, to) (to = value >> 24)
#endif

#endif
