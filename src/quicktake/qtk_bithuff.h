#ifndef __QTK_BITHUFF_H
#define __QTK_BITHUFF_H

#include "platform.h"

extern uint32 bitbuf;
extern uint8 vbits;

void __fastcall__ reset_bitbuff (void);
uint8 __fastcall__ get_four_bits (void);
uint8 __fastcall__ getbithuff (uint8 n);

#endif
