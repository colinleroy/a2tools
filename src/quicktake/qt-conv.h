#ifndef __qt_conv_h
#define __qt_conv_h

#include "platform.h"
#include "extended_conio.h"

#define QT_BAND 20

#define QTKT_MAGIC      "qktk"
#define QTKN_MAGIC      "qktn"
#define JPEG_EXIF_MAGIC "\377\330\377\341"

#define FILE_WIDTH 256
#define FILE_HEIGHT HGR_HEIGHT
#define THUMB_WIDTH 80
#define THUMB_HEIGHT 60

#define QT200_JPEG_WIDTH 320
#define QT200_JPEG_HEIGHT 240

#ifdef __CC65__
#define TMP_NAME "/RAM/GREY"
#define HIST_NAME "/RAM/HIST"
#define THUMBNAIL_NAME "/RAM/THUMBNAIL"
#else
#define TMP_NAME "GREY"
#define HIST_NAME "HIST"
#define THUMBNAIL_NAME "THUMBNAIL"
#endif

extern uint16 height, width;
extern uint8 raw_image[];
extern uint16 raw_width, raw_image_size;

extern char magic[5];
extern char *model;

extern uint32 bitbuf;
extern uint8 vbits;

void __fastcall__ src_file_seek(uint32 off);
uint32 __fastcall__ cache_read_since_inval(void);
extern uint8 cache[];
extern uint16 cache_size;

void __fastcall__ src_file_get_bytes(uint8 *dst, uint16 count);

extern uint16 *huff_ptr;
uint8 __fastcall__ getbithuff (uint8 nbits);
uint8 __fastcall__ getbitnohuff (uint8 nbits);

void qt_load_raw(uint16 top);

#define RAW(row,col) raw_image[((row)*width)+(col)]
#define RAW_IDX(row,col) (((row)*width)+(col))
#define RAW_DIRECT_IDX(idx) raw_image[idx]

#define FILE(row,col) raw_image[((row)*width)+(col)]
#define FILE_IDX(row,col) (((row)*width)+(col))
#define FILE_DIRECT_IDX(idx) raw_image[idx]

#define getbits(n) getbitnohuff(n)

#ifdef __CC65__

#define FAST_SHIFT_LEFT_8_LONG(value) do { \
    __asm__("ldx %v+2", value);    \
    __asm__("stx %v+3", value);    \
    __asm__("ldx %v+1", value);    \
    __asm__("stx %v+2", value);    \
    __asm__("ldx %v", value);      \
    __asm__("stx %v+1", value);    \
    __asm__("stz %v", value);      \
} while (0)
#define FAST_SHIFT_LEFT_16_LONG(value) do {\
    __asm__("ldx %v+1", value);    \
    __asm__("stx %v+3", value);    \
    __asm__("ldx %v", value);      \
    __asm__("stx %v+2", value);    \
    __asm__("stz %v+1", value);    \
    __asm__("stz %v", value);      \
} while(0)
#define FAST_SHIFT_LEFT_24_LONG(value) do {\
    __asm__("ldx %v", value);      \
    __asm__("stx %v+3", value);    \
    __asm__("stz %v+2", value);    \
    __asm__("stz %v+1", value);    \
    __asm__("stz %v", value);      \
} while(0)

#define FAST_SHIFT_RIGHT_8_LONG(value) do { \
    __asm__("ldx %v+1", value);    \
    __asm__("stx %v", value);      \
    __asm__("ldx %v+2", value);    \
    __asm__("stx %v+1", value);    \
    __asm__("ldx %v+3", value);    \
    __asm__("stx %v+2", value);    \
    __asm__("stz %v+3", value);    \
} while(0)
#define FAST_SHIFT_RIGHT_16_LONG(value) do { \
    __asm__("ldx %v+2", value);    \
    __asm__("stx %v", value);      \
    __asm__("ldx %v+3", value);    \
    __asm__("stx %v+1", value);    \
    __asm__("stz %v+2", value);    \
    __asm__("stz %v+3", value);    \
} while(0)
#define FAST_SHIFT_RIGHT_24_LONG(value) do { \
    __asm__("ldx %v+3", value);    \
    __asm__("stx %v", value);      \
    __asm__("stz %v+1", value);    \
    __asm__("stz %v+2", value);    \
    __asm__("stz %v+3", value);    \
} while(0)

#define FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(value) do { \
    __asm__("lda %v+1", value);    \
    __asm__("sta %v", value);      \
    __asm__("ldx %v+2", value);    \
    __asm__("stx %v+1", value);    \
} while(0)
#define FAST_SHIFT_RIGHT_16_LONG_SHORT_ONLY(value) do { \
    __asm__("lda %v+2", value);    \
    __asm__("sta %v", value);      \
    __asm__("ldx %v+3", value);    \
    __asm__("stx %v+1", value);    \
} while(0)
#define FAST_SHIFT_RIGHT_24_LONG_SHORT_ONLY(value) do { \
    __asm__("lda %v+3", value);    \
    __asm__("sta %v", value);      \
    __asm__("stz %v+1", value);    \
} while(0)

#define FAST_SHIFT_RIGHT_8_LONG_CHAR_ONLY(value) do { \
    __asm__("ldx %v+1", value);    \
    __asm__("stx %v", value);      \
} while(0)
#define FAST_SHIFT_RIGHT_16_LONG_CHAR_ONLY(value) do { \
    __asm__("ldx %v+2", value);    \
    __asm__("stx %v", value);      \
} while(0)
#define FAST_SHIFT_RIGHT_24_LONG_CHAR_ONLY(value) do { \
    __asm__("ldx %v+3", value);    \
    __asm__("stx %v", value);      \
} while(0)

#define FAST_SHIFT_LEFT_8_INT_TO_LONG(value, to) do { \
    __asm__("stz %v+3", to);      \
    __asm__("ldx %v+1", value);    \
    __asm__("stx %v+2", to);    \
    __asm__("ldx %v", value);      \
    __asm__("stx %v+1", to);    \
    __asm__("stz %v", to);      \
} while (0)

#define FAST_SHIFT_LEFT_8_LONG_TO(value, to) do { \
    __asm__("ldx %v+2", value);    \
    __asm__("stx %v+3", to);    \
    __asm__("ldx %v+1", value);    \
    __asm__("stx %v+2", to);    \
    __asm__("ldx %v", value);      \
    __asm__("stx %v+1", to);    \
    __asm__("stz %v", to);      \
} while (0)
#define FAST_SHIFT_LEFT_16_LONG_TO(value, to) do {\
    __asm__("ldx %v+1", value);    \
    __asm__("stx %v+3", to);    \
    __asm__("ldx %v", value);      \
    __asm__("stx %v+2", to);    \
    __asm__("stz %v+1", to);    \
    __asm__("stz %v", to);      \
} while(0)
#define FAST_SHIFT_LEFT_24_LONG_TO(value, to) do {\
    __asm__("ldx %v", value);      \
    __asm__("stx %v+3", to);    \
    __asm__("stz %v+2", to);    \
    __asm__("stz %v+1", to);    \
    __asm__("stz %v", to);      \
} while(0)

#define FAST_SHIFT_RIGHT_8_LONG_TO(value, to) do { \
    __asm__("ldx %v+1", value);    \
    __asm__("stx %v", to);      \
    __asm__("ldx %v+2", value);    \
    __asm__("stx %v+1", to);    \
    __asm__("ldx %v+3", value);    \
    __asm__("stx %v+2", to);    \
    __asm__("stz %v+3", to);    \
} while(0)
#define FAST_SHIFT_RIGHT_16_LONG_TO(value, to) do { \
    __asm__("ldx %v+2", value);    \
    __asm__("stx %v", to);      \
    __asm__("ldx %v+3", value);    \
    __asm__("stx %v+1", to);    \
    __asm__("stz %v+2", to);    \
    __asm__("stz %v+3", to);    \
} while(0)
#define FAST_SHIFT_RIGHT_24_LONG_TO(value, to) do { \
    __asm__("ldx %v+3", value);    \
    __asm__("stx %v", to);      \
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
#define FAST_SHIFT_RIGHT_8_LONG_SHORT_ONLY(value) (value >>=8)
#define FAST_SHIFT_RIGHT_16_LONG_SHORT_ONLY(value) (value >>=16)
#define FAST_SHIFT_RIGHT_24_LONG_SHORT_ONLY(value) (value >>=24)
#define FAST_SHIFT_RIGHT_8_LONG_CHAR_ONLY(value) (value >>=8)
#define FAST_SHIFT_RIGHT_16_LONG_CHAR_ONLY(value) (value >>=16)
#define FAST_SHIFT_RIGHT_24_LONG_CHAR_ONLY(value) (value >>=24)

#define FAST_SHIFT_LEFT_8_INT_TO_LONG(value, to) (to = value << 8)
#define FAST_SHIFT_LEFT_8_LONG_TO(value, to) (to = value << 8)
#define FAST_SHIFT_LEFT_16_LONG_TO(value, to) (to = value << 16)
#define FAST_SHIFT_LEFT_24_LONG_TO(value, to) (to = value << 24)
#define FAST_SHIFT_RIGHT_8_LONG_TO(value, to) (to = value >> 8)
#define FAST_SHIFT_RIGHT_16_LONG_TO(value, to) (to = value >> 16)
#define FAST_SHIFT_RIGHT_24_LONG_TO(value, to) (to = value >> 24)
#endif

#endif
