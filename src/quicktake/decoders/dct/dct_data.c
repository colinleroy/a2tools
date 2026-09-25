#include "platform.h"
#include "dct_platform.h"
#include "../qt-conv.h"

uint8 cache[CACHE_SIZE];
uint8 raw_image[RAW_IMAGE_SIZE];
uint8 *idx0, *idx1, *idx2, *idx3, *idx4, *idx5, *idx6, *idx7;

uint8 numbits;
uint8 bitpos;
uint8 valneg;
int8 bitval;

int8 coef[128];
uint8 scan;
uint8 ob;

/* Doubled for index convenience */
uint8 SCAN[64] = {
     0*2, 1*2, 8*2,16*2, 9*2, 2*2, 3*2,10*2,
    17*2,24*2,32*2,25*2,18*2,11*2, 4*2, 5*2,
    12*2,19*2,26*2,33*2,40*2,48*2,41*2,34*2,
    27*2,20*2,13*2, 6*2, 7*2,14*2,21*2,28*2,
    35*2,42*2,49*2,56*2,57*2,50*2,43*2,36*2,
    29*2,22*2,15*2,23*2,30*2,37*2,44*2,51*2,
    58*2,59*2,52*2,45*2,38*2,31*2,39*2,46*2,
    53*2,60*2,61*2,54*2,47*2,55*2,62*2,63*2,
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

/* bits : how many bits to read */
uint8 normal_bits[64] = {
    8, 8, 8, 7, 7, 7, 7, 7,
    7, 7, 6, 5, 6, 7, 7, 7,
    6, 5, 5, 5, 5, 3, 0, 0,
    5, 5, 5, 6, 6, 5, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0,
    4, 4, 4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

/* shift: how many times to shift right after getting bits,
 * to align them right while shifting >> 3.
 */
uint8 normal_shift[64] = {
    1, 1, 1, 2, 2, 2, 2, 2,
    2, 2, 3, 3, 3, 2, 2, 2,
    3, 3, 3, 3, 4, 5, 0, 0,
    3, 3, 3, 3, 3, 3, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0,
    4, 4, 4, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

uint8 superfine_bits[64] = {
    10,10,10,9, 9, 9, 8, 8,
    8, 8, 7, 7, 7, 8, 8, 8,
    7, 7, 7, 7, 6, 5, 6, 6,
    7, 7, 7, 7, 7, 7, 6, 6,
    6, 6, 5, 4, 4, 4, 5, 6,
    6, 6, 6, 6, 6, 6, 5, 4,
    3, 3, 3, 5, 6, 6, 5, 4,
    3, 3, 3, 3, 4, 3, 3, 3
};

uint8 superfine_shift[64] = {
    1, 1, 1, 2, 2, 2, 2, 2,
    2, 2, 3, 3, 3, 2, 2, 2,
    3, 3, 3, 3, 4, 5, 4, 4,
    3, 3, 3, 3, 3, 3, 4, 4,
    4, 4, 5, 6, 6, 6, 5, 4,
    4, 4, 4, 4, 4, 4, 5, 6,
    6, 6, 6, 5, 4, 4, 5, 5,
    6, 6, 6, 6, 5, 6, 6, 6
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
