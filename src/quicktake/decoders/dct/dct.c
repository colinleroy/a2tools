/* 6502-friendly decoder for the Dycam 10-C. Based on the
 * es3000_decode.py included in this directory, which is
 * an LLM-generated reverse-engineer of the Amiga binary
 * (https://aminet.net/package/driver/other/ES3000_Demo)
 * that has been provided to me.
 * 
 * Of course full standard DCT based on doubles was never
 * going to be okay so DCT core replaced by a Loeffler-
 * Ligtenberg-Moschytz implementation.
 * http://akuvian.org/src/x264/Loeffler_dct.pdf.gz
 * Descaled so 8-bit * maths is possible, and image building
 * rewritten to avoid indexes as much as possible.
 *
 * TBD: write it in assembly.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dct_data.h"
#include "../qt-conv.h"
#include "platform.h"
#include "dct_platform.h"

extern uint32 data_size;

char *decoder_name = "Chinon DCT";

#ifndef __CC65__
uint8 *cache_start = cache;
#endif
uint32 nmults = 0;

#define DESCALE_FACTOR 1

int8 coef[64];
int8 row_out[128]; /* Twice as large as needed but simplifies computations. */
int8 tmp0, tmp1, tmp2, tmp3;
int8 tmp4, tmp5, tmp6, tmp7;
int8 tmp10, tmp11, tmp12, tmp13;
int8 z5, z10, z11, z12, z13;
static void idct_common(void) {
        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;

        if (tmp12) {
          tmp12 = mul_362(tmp12);
        }
        tmp12 -= tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        tmp7 = z11 + z13;

        if (z11 - z13) {
          tmp11 = mul_362(z11 - z13);
          nmults++;
        } else {
          tmp11 = 0;
        }

        if (z10) {
          z13 = mul_669(z10);
          nmults++;
        } else {
          z13 = 0;
        }

        if (z10 + z12) {
          z5 = mul_473(z10 + z12);
          nmults++;
        } else {
          z5 = 0;
        }
        tmp12 = z5 - z13;

        if (z12) {
          tmp10 = mul_277(z12);
          nmults++;
        } else {
          tmp10 = 0;
        }
        tmp10 -= z5;

        tmp6 = tmp12 - tmp7;
        tmp5 = tmp11 - tmp6;
        tmp4 = tmp10 + tmp5;
}

static void idct_1d_rows(void) {
    uint8 y;
    
    for (y = 0; y < 8*16; y+=16) {
        const int8 *x = &coef[y >> 1];

        if (x[1] == 0 && x[2] == 0 &&
            x[3] == 0 && x[4] == 0 &&
            x[5] == 0 && x[6] == 0 &&
            x[7] == 0) {

            row_out[y + 0] =
              row_out[y + 2] =
              row_out[y + 4] =
              row_out[y + 6] =
              row_out[y + 8] =
              row_out[y + 10] =
              row_out[y + 12] =
              row_out[y + 14] = x[0];
            continue;
        }

        tmp10 = x[0] + x[4];
        tmp11 = x[0] - x[4];
        tmp12 = x[2] - x[6];
        tmp13 = x[2] + x[6];
        z10   = x[5] - x[3];
        z11   = x[1] + x[7];
        z12   = x[1] - x[7];
        z13   = x[5] + x[3];

        idct_common();

        row_out[y + 0] = (uint8)(tmp0 + tmp7);
        row_out[y + 2] = (uint8)(tmp1 + tmp6);
        row_out[y + 4] = (uint8)(tmp2 + tmp5);
        row_out[y + 6] = (uint8)(tmp3 - tmp4);
        row_out[y + 8] = (uint8)(tmp3 + tmp4);
        row_out[y + 10] = (uint8)(tmp2 - tmp5);
        row_out[y + 12] = (uint8)(tmp1 - tmp6);
        row_out[y + 14] = (uint8)(tmp0 - tmp7);
    }
}

#define CLAMPU(x) (((uint16)(x) << DESCALE_FACTOR) > 255 ? 255 : ((x)<<DESCALE_FACTOR))

static void idct_1d_cols(void) {
    uint8 x;
    for (x = 0; x < 16; x+=2) {

        if (row_out[x + 16] == 0 && row_out[x + 32] == 0 &&
            row_out[x + 48] == 0 && row_out[x + 64] == 0 &&
            row_out[x + 80] == 0 && row_out[x + 96] == 0 &&
            row_out[x + 112] == 0) {

            if (actual_width == 160) {
              idx[x + 0*RAW_WIDTH] =
                idx[x + 1*RAW_WIDTH] =
                idx[x + 2*RAW_WIDTH] =
                idx[x + 3*RAW_WIDTH] =
                idx[x + 4*RAW_WIDTH] =
                idx[x + 5*RAW_WIDTH] =
                idx[x + 6*RAW_WIDTH] =
                idx[x + 7*RAW_WIDTH] =
              idx[x + 1 + 0*RAW_WIDTH] =
                idx[x + 1 + 1*RAW_WIDTH] =
                idx[x + 1 + 2*RAW_WIDTH] =
                idx[x + 1 + 3*RAW_WIDTH] =
                idx[x + 1 + 4*RAW_WIDTH] =
                idx[x + 1 + 5*RAW_WIDTH] =
                idx[x + 1 + 6*RAW_WIDTH] =
                idx[x + 1 + 7*RAW_WIDTH] = CLAMPU(row_out[x + 0]);
            } else {
              idx[x/2 + 0*RAW_WIDTH] = 
                idx[x/2 + 1*RAW_WIDTH] = 
                idx[x/2 + 2*RAW_WIDTH] = 
                idx[x/2 + 3*RAW_WIDTH] = CLAMPU(row_out[x + 0]);
            }
            
        } else {
            tmp10 = row_out[x + 0] + row_out[x + 64];
            tmp11 = row_out[x + 0] - row_out[x + 64];
            tmp12 = row_out[x+32] - row_out[x + 96];
            tmp13 = row_out[x + 32] + row_out[x + 96];
            z10 = row_out[x + 80] - row_out[x + 48];
            z11 = row_out[x + 16] + row_out[x + 112];
            z12 = row_out[x + 16] - row_out[x + 112];
            z13 = row_out[x + 80] + row_out[x + 48];

            idct_common();

            if (actual_width == 160) {
              idx[x + 0*RAW_WIDTH] = idx[x + 1 + 0*RAW_WIDTH] = CLAMPU(tmp0 + tmp7);
              idx[x + 2*RAW_WIDTH] = idx[x + 1 + 2*RAW_WIDTH] = CLAMPU(tmp2 + tmp5);
              idx[x + 4*RAW_WIDTH] = idx[x + 1 + 4*RAW_WIDTH] = CLAMPU(tmp3 + tmp4);
              idx[x + 6*RAW_WIDTH] = idx[x + 1 + 6*RAW_WIDTH] = CLAMPU(tmp1 - tmp6);
              
              idx[x + 1*RAW_WIDTH] = idx[x + 1 + 1*RAW_WIDTH] = CLAMPU(tmp1 + tmp6);
              idx[x + 3*RAW_WIDTH] = idx[x + 1 + 3*RAW_WIDTH] = CLAMPU(tmp3 - tmp4);
              idx[x + 5*RAW_WIDTH] = idx[x + 1 + 5*RAW_WIDTH] = CLAMPU(tmp2 - tmp5);
              idx[x + 7*RAW_WIDTH] = idx[x + 1 + 7*RAW_WIDTH] = CLAMPU(tmp0 - tmp7);
            } else {
              idx[x/2 + 0*RAW_WIDTH] = CLAMPU(tmp0 + tmp7);
              idx[x/2 + 1*RAW_WIDTH] = CLAMPU(tmp2 + tmp5);
              idx[x/2 + 2*RAW_WIDTH] = CLAMPU(tmp3 + tmp4);
              idx[x/2 + 3*RAW_WIDTH] = CLAMPU(tmp1 - tmp6);
            }
        }
    }
}

uint8 qt_load_raw(uint16 top)
{
    uint8 blocks_rem_in_row = blocks_per_row;
    uint16 block;
    uint8 scan;
    idx = raw_image;

    for (block = 0; block < blocks_per_band; block++) {
        for (scan = 0; scan < 64; scan++) {
            uint8 r = SCAN[scan];
            uint8 ob;

            if (!(numbits = bits_table[scan])) {
                coef[r] = 0;
                continue;
            }

            bitpos = shift_table[scan];
            /* get_bitval will destroy bitpos */
            ob = numbits + bitpos;

            get_bitval();

            /* extend sign bit */
            if (scan && valneg) {
                bitval = (uint16)bitval | (negate_h[ob] << 8) | negate_l[ob];
            }
            coef[r] = (int8)(bitval >> (DESCALE_FACTOR+2));
        }

        idct_1d_rows();
        idct_1d_cols();

        if (--blocks_rem_in_row == 0) {
          uint8 *old_idx = idx;
          if (actual_width == 160) {
            idx += 8*RAW_WIDTH-DECODE_WIDTH+16;
          } else {
            idx += 4*RAW_WIDTH-DECODE_WIDTH+8;
          }
          blocks_rem_in_row = blocks_per_row;
        } else {
          idx += actual_width == 160 ? 16 : 8;
        }

        /* Each block consumes a constant number of full bytes */
        // assert(nbits_avail == 0);
    }
    return 0;
}
