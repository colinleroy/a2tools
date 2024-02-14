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
#include "extrazp.h"
#else
  #ifdef __APPLE2__
    #include <apple2.h>
    #include "extrazp.h"
  #else
    #include "extended_conio.h"
  #endif
#endif
#endif

#ifdef __APPLE2__
void __fastcall__ platform_msleep(uint16 ms);
#else
#define platform_msleep(n) usleep(n*1000)
#define beep()
#endif
