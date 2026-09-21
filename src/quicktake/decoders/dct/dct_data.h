#ifndef __DCT_DATA_H
#define __DCT_DATA_H

#include "platform.h"

extern uint16 actual_width;
extern uint8 normal_bits[64];
extern uint8 superfine_bits[64];
extern uint8 normal_shift[64];
extern uint8 superfine_shift[64];

extern uint8 *bits_table;
extern uint8 *shift_table;

extern uint16 total_blocks;
extern uint16 blocks_per_band;
extern uint8 blocks_per_row;

#endif
