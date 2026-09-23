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

#define CLAMPU(x) (((uint16)(x) << DESCALE_FACTOR) > 255 ? 255 : ((x)<<DESCALE_FACTOR))

static void idct_1d_cols(void) {
    uint8 x;
    for (x = 0; x < 16; x+=2) {

        if (row_out[x + 16] == 0 && row_out[x + 32] == 0 &&
            row_out[x + 48] == 0 && row_out[x + 64] == 0 &&
            row_out[x + 80] == 0 && row_out[x + 96] == 0 &&
            row_out[x + 112] == 0) {

            if (actual_width == 160) {
              idx0[x] =
                idx1[x] =
                idx2[x] =
                idx3[x] =
                idx4[x] =
                idx5[x] =
                idx6[x] =
                idx7[x] =
              idx0[x + 1] =
                idx1[x + 1] =
                idx2[x + 1] =
                idx3[x + 1] =
                idx4[x + 1] =
                idx5[x + 1] =
                idx6[x + 1] =
                idx7[x + 1] = CLAMPU(row_out[x + 0]);
            } else {
              idx0[x/2] =
                idx1[x/2] =
                idx2[x/2] =
                idx3[x/2] = CLAMPU(row_out[x + 0]);
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
              idx0[x] = idx0[x + 1] = CLAMPU(tmp0 + tmp7);
              idx1[x] = idx1[x + 1] = CLAMPU(tmp2 + tmp5);
              idx2[x] = idx2[x + 1] = CLAMPU(tmp3 + tmp4);
              idx3[x] = idx3[x + 1] = CLAMPU(tmp1 - tmp6);

              idx4[x] = idx4[x + 1] = CLAMPU(tmp1 + tmp6);
              idx5[x] = idx5[x + 1] = CLAMPU(tmp3 - tmp4);
              idx6[x] = idx6[x + 1] = CLAMPU(tmp2 - tmp5);
              idx7[x] = idx7[x + 1] = CLAMPU(tmp0 - tmp7);
            } else {
              idx0[x/2] = CLAMPU(tmp0 + tmp7);
              idx1[x/2] = CLAMPU(tmp2 + tmp5);
              idx2[x/2] = CLAMPU(tmp3 + tmp4);
              idx3[x/2] = CLAMPU(tmp1 - tmp6);
            }
        }
    }
}

uint8 qt_load_raw(uint16 top)
{
    uint16 block;

    idx0 = raw_image;
    idx1 = idx0 + RAW_WIDTH;
    idx2 = idx1 + RAW_WIDTH;
    idx3 = idx2 + RAW_WIDTH;
    idx4 = idx3 + RAW_WIDTH;
    idx5 = idx4 + RAW_WIDTH;
    idx6 = idx5 + RAW_WIDTH;
    idx7 = idx6 + RAW_WIDTH;
    blocks_rem_in_row = blocks_per_row;

    for (block = 0; block < blocks_per_band; block++) {
        get_coeffs();
        idct_1d_rows();
        idct_1d_cols();
        advance_block();

        /* Each block consumes a constant number of full bytes */
        // assert(nbits_avail == 0);
    }
    return 0;
}
