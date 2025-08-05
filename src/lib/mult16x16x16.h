#ifndef __MULT16x16x16_H
#define __MULT16x16x16_H

// Warning: requires aligned DATA tables

#ifdef __CC65__
unsigned long __fastcall__ mult16x16x16(unsigned int a, unsigned int b);
#else
#define mult16x16x16(a,b) (a)*(b)
#endif

#endif
