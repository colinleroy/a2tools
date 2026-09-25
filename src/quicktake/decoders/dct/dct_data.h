#ifndef __DCT_DATA_H
#define __DCT_DATA_H

#include "platform.h"
#include "../qt-conv.h"

extern uint8 cache[CACHE_SIZE];
extern uint8 *cache_start;
extern uint8 *cache_read;
extern uint8 raw_image[RAW_IMAGE_SIZE];
extern uint8 *idx0;
extern uint8 *idx1;
extern uint8 *idx2;
extern uint8 *idx3;
extern uint8 *idx4;
extern uint8 *idx5;
extern uint8 *idx6;
extern uint8 *idx7;

extern uint8 numbits;
extern uint8 valneg;
extern int16 bitval;

extern uint8 SCAN[64];
extern int8 coef[64];
extern uint8 scan;
extern uint8 ob;

extern int8 nbits_avail;
#pragma zpsym("nbits_avail")

extern uint8 bitmask[8];
extern uint8 negate[8];

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

extern int8 row_out[128]; /* Twice as large as needed but simplifies computations. */
extern int8 tmp0, tmp1, tmp2, tmp3;
extern int8 tmp4, tmp5, tmp6, tmp7;
extern int8 tmp10, tmp11, tmp12, tmp13;
extern int8 z5, z10, z11, z12, z13;

#endif
