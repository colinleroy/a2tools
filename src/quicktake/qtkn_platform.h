#ifndef __QTKN_PLATFORM_H
#define __QTKN_PLATFORM_H

#include "platform.h"

#define DATABUF_SIZE 322

extern uint8 val_hi_from_last[17];
extern uint8 val_from_last[256];
extern const uint8 src[226];

void consume_extra(void);
void init_row(void);
void decode_row(void);

void init_top(void);
void init_shiftl4(void);
void init_shiftl3(void);
void init_div48(void);
void init_buf_0(void);
void init_huff(void);

#endif
