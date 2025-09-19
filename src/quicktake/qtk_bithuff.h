#ifndef __QTK_BITHUFF_H
#define __QTK_BITHUFF_H

#include "platform.h"

extern uint8 huff_num;
extern uint8 huff_split[18*2][256];

void __fastcall__ reset_bitbuff (void);
uint8 __fastcall__ get_four_bits (void);
uint8 __fastcall__ getbits6 (void);
uint8 __fastcall__ getbithuff (void);
uint8 __fastcall__ getbithuff36 (void);
void __fastcall__ initbithuff (void);
#ifdef __CC65__
void init_floppy_starter(void);
#endif

#endif
