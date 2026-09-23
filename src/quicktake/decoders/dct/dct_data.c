#include "platform.h"
#include "dct_platform.h"
#include "../qt-conv.h"

uint8 cache[CACHE_SIZE];
uint8 raw_image[RAW_IMAGE_SIZE];
uint8 *idx;

uint8 numbits;
uint8 bitpos;
uint8 valneg;
int8 bitval;

int8 coef[64];
uint8 scan;
uint8 ob;

uint8 SCAN[64] = {
     0, 1, 8,16, 9, 2, 3,10,
    17,24,32,25,18,11, 4, 5,
    12,19,26,33,40,48,41,34,
    27,20,13, 6, 7,14,21,28,
    35,42,49,56,57,50,43,36,
    29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,
    53,60,61,54,47,55,62,63,
};

int8 nbits_avail;

uint8 bitmask[8] = {
  0b00000001,
  0b00000010,
  0b00000100,
  0b00001000,
  0b00010000,
  0b00100000,
  0b01000000,
  0b10000000,
};

uint8 negate[8] = {
  0b11111111,
  0b11111110,
  0b11111100,
  0b11111000,
  0b11110000,
  0b11100000,
  0b11000000,
  0b10000000,
};

uint8 normal_bits[64] = {
    8,8,8,7,7,7,7,7,
    7,7,6,5,6,7,7,7,
    6,5,5,5,5,3,0,0,
    5,5,5,6,6,5,4,4,
    0,0,0,0,0,0,0,0,
    4,4,4,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

#define IGNORE_BITS(x) (DESCALE_FACTOR+2-(x))
uint8 normal_shift[64] = {
    IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2),
    IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(3), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2),
    IGNORE_BITS(2), IGNORE_BITS(3), IGNORE_BITS(3), IGNORE_BITS(3), IGNORE_BITS(2), IGNORE_BITS(3), IGNORE_BITS(0), IGNORE_BITS(0),
    IGNORE_BITS(3), IGNORE_BITS(3), IGNORE_BITS(3), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(3), IGNORE_BITS(3), IGNORE_BITS(3),
    IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0),
    IGNORE_BITS(3), IGNORE_BITS(3), IGNORE_BITS(3), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0),
    IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0),
    IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0)
};

uint8 superfine_bits[64] = {
    10,10,10,9,9,9,8,8,
    8,8,7,7,7,8,8,8,
    7,7,7,7,6,5,6,6,
    7,7,7,7,7,7,6,6,
    6,6,5,4,4,4,5,6,
    6,6,6,6,6,6,5,4,
    3,3,3,5,6,6,5,4,
    3,3,3,3,4,3,3,3
};

uint8 superfine_shift[64] = {
    IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(0), IGNORE_BITS(1), IGNORE_BITS(1),
    IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1),
    IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1),
    IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1),
    IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1),
    IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1),
    IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(1), IGNORE_BITS(2),
    IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2), IGNORE_BITS(2)
};

#ifdef __CC65__
uint8 histogram_low[256];
uint8 histogram_high[256];
uint8 orig_x_offset[256];
uint8 special_x_orig_offset[256];
uint8 orig_y_table_l[256];
uint8 orig_y_table_h[256];
#endif

uint8 *bits_table;
uint8 *shift_table;

uint16 actual_width;
uint16 total_blocks;
uint16 blocks_per_band;
uint8 blocks_per_row;
uint8 blocks_rem_in_row;

int8 row_out[128]; /* Twice as large as needed but simplifies computations. */
int8 tmp0, tmp1, tmp2, tmp3;
int8 tmp4, tmp5, tmp6, tmp7;
int8 tmp10, tmp11, tmp12, tmp13;
int8 z5, z10, z11, z12, z13;
