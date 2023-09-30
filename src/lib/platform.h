#ifndef __platform_h
#define __platform_h

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
