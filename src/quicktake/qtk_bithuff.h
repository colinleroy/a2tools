#ifndef __QTK_BITHUFF_H
#define __QTK_BITHUFF_H

#include "platform.h"

void __fastcall__ reset_bitbuff (void);
uint8 __fastcall__ get_four_bits (void);
uint8 __fastcall__ getbithuff (uint8 n);
uint8 __fastcall__ getbithuff6 (void);
uint8 __fastcall__ getbithuff8 (void);
void __fastcall__ initbithuff (void);
#ifdef __CC65__
void init_floppy_starter(void);
#endif

#endif
