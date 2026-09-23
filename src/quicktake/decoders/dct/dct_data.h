#ifndef __DCT_DATA_H
#define __DCT_DATA_H

#include "platform.h"
#include "../qt-conv.h"

extern uint8 cache[CACHE_SIZE];
extern uint8 *cache_start;
extern uint8 *cache_read;
extern uint8 raw_image[RAW_IMAGE_SIZE];
extern uint8 *idx;

extern uint8 numbits;
extern uint8 bitpos;
extern uint8 valneg;
extern int16 bitval;

extern uint8 SCAN[64];
extern int8 coef[64];
extern uint8 scan;
extern uint8 ob;

extern int8 nbits_avail;
extern uint8 bitmask_h[16];
extern uint8 bitmask_l[16];
extern uint8 negate_h[16];
extern uint8 negate_l[16];

extern uint8 normal_bits[64];
extern uint8 superfine_bits[64];
extern uint8 normal_shift[64];
extern uint8 superfine_shift[64];

extern uint8 *bits_table;
extern uint8 *shift_table;

extern uint16 actual_width;
extern uint16 total_blocks;
extern uint16 blocks_per_band;
extern uint8 blocks_per_row;
extern uint8 blocks_rem_in_row;
#endif
