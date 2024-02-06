#ifndef __APPROXDIV16x8_H
#define __APPROXDIV16x8_H

// Warning: requires aligned DATA tables

#ifdef __CC65__
unsigned int __fastcall__ approx_div16x8(unsigned int a, unsigned char b);
#else
#define approx_div16x8(a,b) (a)/(b)
#endif

#endif
