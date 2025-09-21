#ifndef __QTKN_PLATFORM_H
#define __QTKN_PLATFORM_H

#include "platform.h"

#define WIDTH 640U
#define HEIGHT 480U
#define USEFUL_DATABUF_SIZE 321
#define DATABUF_SIZE 0x400 /* align things */
#define SET_CURBUF_VAL(bufl, bufh, y, val) do { int16 v = (int16)(val); *(uint8 *)((bufl)+(y)) = (v)&0xff; *(uint8 *)((bufh)+(y)) = (v)>>8; } while (0)
#define GET_CURBUF_VAL(bufl, bufh, y) ((int16)(((uint8)( *((bufl)+(y)) ))|(((uint8)( *(((bufh))+(y)) ))<<8)))

extern uint8 val_hi_from_last[17];
extern uint8 val_from_last[256];

void copy_data(void);
void consume_extra(void);
void init_row(void);
void decode_row(void);

void init_shiftl4(void);
#endif
