#ifndef __lut_mul_h
#define __lut_mul_h

#include "platform.h"

#ifdef __CC65__
uint16 __fastcall__ lut_mul (uint8 a, uint8 b);
int16 __fastcall__  lut_smul(int8 a,  int8 b);
#else
#define lut_mul(a,b) ((a)*(b))
#define lut_smul(a,b) ((a)*(b))
#endif

#endif
