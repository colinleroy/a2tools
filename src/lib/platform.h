#ifndef __platform_h
#define __platform_h

#ifdef __CC65__
  #define uint8  unsigned char
  #define uint16 unsigned int
  #define uint32 unsigned long
  #define int8  signed char
  #define int16 signed int
  #define int32 signed long
#else
  #define uint8  unsigned char
  #define uint16 unsigned short
  #define uint32 unsigned int
  #define int8  signed char
  #define int16 signed short
  #define int32 signed int
  #define __fastcall__
#endif

#ifdef __APPLE2ENH__
#include <apple2enh.h>
#else
  #ifdef __APPLE2__
    #include <apple2.h>
  #else
    #include "extended_conio.h"
  #endif
#endif
#endif

#ifdef __APPLE2__
void platform_sleep(uint8 s);
void platform_msleep(uint16 ms);
#else
#define platform_sleep(n) sleep(n)
#define platform_msleep(n) usleep(n*1000)
#endif

#ifdef IIGS

extern uint8 orig_speed_reg;

#define slowdown() do {               \
  __asm__("lda $C036"); /* CYAREG */  \
  __asm__("sta %v", orig_speed_reg);  \
  __asm__("and #$79");                \
  __asm__("sta $C036");               \
} while (0)

#define speedup() do {                \
  __asm__("lda %v", orig_speed_reg);  \
  __asm__("sta $C036");               \
} while (0)

#else
#define slowdown()
#define speedup()
#endif
