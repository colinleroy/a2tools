#include "platform.h"

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

uint8 normal_shift[64] = {
    2,2,2,2,2,2,2,2,
    2,2,2,3,2,2,2,2,
    2,3,3,3,2,3,7,7,
    3,3,3,2,2,3,3,3,
    7,7,6,5,5,5,6,7,
    3,3,3,7,7,7,6,5,
    5,5,5,6,7,7,6,6,
    5,5,5,5,6,5,5,5
};

uint8 superfine_shift[64] = {
    0,0,0,0,0,0,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    2,2,2,1,1,1,1,2,
    2,2,2,2,2,2,2,2
};

char *decoder_name = "Chinon DCT";

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
