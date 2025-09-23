#ifndef __QTK_BITHUFF_H
#define __QTK_BITHUFF_H

#include "platform.h"
#include "qtkn_platform.h"

extern uint8 huff_num;
extern uint8 huff_numc, huff_numc_h;
extern uint8 huff_numd, huff_numd_h;
extern uint8 huff_split[18*2][256];
extern uint8 huff_ctrl[9*2][256];
extern uint8 huff_data[9][256];
extern uint8 buf_0[DATABUF_SIZE];
extern uint8 buf_1[DATABUF_SIZE];
extern uint8 buf_2[DATABUF_SIZE];
extern uint8 shiftl4p_l[128];
extern uint8 shiftl4p_h[128];
extern uint8 shiftl4n_l[128];
extern uint8 shiftl4n_h[128];
extern uint8 shiftl3[32];
extern uint8 div48_l[256];
extern uint8 div48_h[256];
extern uint8 dyndiv_l[256];
extern uint8 dyndiv_h[256];

void __fastcall__ reset_bitbuff (void);
uint8 __fastcall__ get_four_bits (void);
uint8 __fastcall__ getbits6 (void);
uint8 __fastcall__ getctrlhuff (void);
uint8 __fastcall__ getdatahuff (void);
uint8 __fastcall__ getdatahuff8 (void);
void __fastcall__ initbithuff (void);
#ifdef __CC65__
void init_floppy_starter(void);
#endif

#endif
