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

uint8 qt_load_raw(uint16 top)
{
    uint16 block;

    init_idx();
    update_idx();
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
