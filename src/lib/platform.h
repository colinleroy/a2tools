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
void platform_sleep(int n);
#else
#define platform_sleep(n) sleep(n)
#endif
