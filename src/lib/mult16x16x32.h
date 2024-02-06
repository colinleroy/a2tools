#ifndef __MULT16x16x32_H
#define __MULT16x16x32_H

// Warning: requires aligned DATA tables

#ifdef __CC65__
unsigned long __fastcall__ mult16x16x32(unsigned int a, unsigned int b);
#else
#define mult16x16x32(a,b) (a)*(b)
#endif

#endif
