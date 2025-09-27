#ifndef __QTK_BITHUFF_H
#define __QTK_BITHUFF_H

#include "platform.h"
#include "qtkn_platform.h"

extern int16 next_line[DATABUF_SIZE];
extern uint8 huff_split[18*2][256];
extern uint8 huff_ctrl[9*2][256];
extern uint8 huff_data[9][256];
extern uint8 div48_l[256];
extern uint8 div48_h[256];
extern uint8 dyndiv_l[256];
extern uint8 dyndiv_h[256];

void __fastcall__ reset_bitbuff (void);
uint8 __fastcall__ get_four_bits (void);
uint8 __fastcall__ getbits6 (void);
uint8 __fastcall__ getctrlhuff (uint8 huff_num);
uint8 __fastcall__ getdatahuff (uint8 huff_num);
uint8 __fastcall__ getdatahuff8 (void);
void __fastcall__ initbithuff (void);
#ifdef __CC65__
void init_floppy_starter(void);
#endif

#endif
