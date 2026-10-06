#include <unistd.h>
#include <assert.h>

#include "platform.h"
#include "dct_data.h"
#include "dct_platform.h"

static int8 mul_362(int8 w)
{
  int16 x;
  x = (int16)(w * 362);
  x >>= 8;
  return (int8)x & 0xFF;
}

static int8 mul_473(int8 w)
{
  int16 x;
  x = (int16)(w * 473);
  x >>= 8;
  return (int8)x & 0xFF;
}

static int8 mul_277(int8 w)
{
  int16 x;
  x = (int16)(w * 277);
  x >>= 8;
  return (int8)x & 0xFF;
}

static int8 mul_669(int8 w)
{
  int16 x;
  x = (int16)(w * 669);
  x >>= 8;
  return (int8)x & 0xFF;
}

int8 ign_bits;

void get_coeffs(void) {
  if (cur_cache_ptr == cache_end) {
      read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
  }
  nbits_avail = 8;
  for (scan = 0; scan < 64; scan++) {
      uint8 r = SCAN[scan];

      if (!(numbits = bits_table[scan])) {
          continue;
      }
      ign_bits = shift_table[scan];

      get_bitval();
      coef[r] = (int8)(bitval);
  }
  assert(nbits_avail == 0);
  cur_cache_ptr++;
}

void get_bitval(void) {
  bitval = valneg = 0;

  do {
    if (--nbits_avail < 0) {
        nbits_avail = 7;
        cur_cache_ptr++;
    }

    valneg = *cur_cache_ptr & 1;
    *cur_cache_ptr >>= 1;
    bitval >>= 1;
    if (valneg) {
      bitval |= 0x80;
    }
  } while (--numbits);

  if (!scan) {
    goto shift_pos;
  } else if (valneg) {
    do {
      bitval >>= 1;
      bitval |= 0x80;
    } while (--ign_bits);
    return;
  }
shift_pos:
  do {
    bitval >>= 1;
  } while (--ign_bits);
}

void advance_block(void) {
  if (--blocks_rem_in_row == 0) {
    if (image_size == 0) {
      idx0 += 8*RAW_WIDTH-DECODE_WIDTH+16;
    } else {
      idx0 += 4*RAW_WIDTH-DECODE_WIDTH+8;
    }
    blocks_rem_in_row = blocks_per_row;
  } else {
    idx0 += image_size == 0 ? 16 : 8;
  }
  update_idx();
}

#define CLAMPI(x) ((x < -128 ? -128 : (x > 127 ? 127 : x)))
#define CLAMPU(x) (((x) < 0) ? 0 : ((x) > 127 ? 255 : ((x)<<DESCALE_FACTOR)))

/* AAN (Arai, Agui, and Nakajima) IDCT, scaled down to int8 and artifacting
 * but not enough to ruin the dithered output */
void idct_1d_rows(void) {
    uint8 y;

    for (y = 0; y < 8*16; y+=16) {
        int8 z13_0, z13_1;
        if (coef[y + 2] == 0 && coef[y + 4] == 0 &&
            coef[y + 6] == 0 && coef[y + 8] == 0 &&
            coef[y + 10] == 0 && coef[y + 12] == 0 &&
            coef[y + 14] == 0) {

            row_out[y + 0] =
              row_out[y + 2] =
              row_out[y + 4] =
              row_out[y + 6] =
              row_out[y + 8] =
              row_out[y + 10] =
              row_out[y + 12] =
              row_out[y + 14] = coef[y + 0];
            continue;
        }

        tmp13 = CLAMPI(coef[y + 4] + coef[y + 12]);

        tmp10 = CLAMPI(coef[y + 0] + coef[y + 8]);
        tmp0 = CLAMPI(tmp10 + tmp13);
        tmp3 = CLAMPI(tmp10 - tmp13);

        tmp11 = CLAMPI(coef[y + 0] - coef[y + 8]);
        tmp12 = CLAMPI(mul_362(CLAMPI(coef[y + 4] - coef[y + 12])) - tmp13);
        tmp1 = CLAMPI(tmp11 + tmp12);
        tmp2 = CLAMPI(tmp11 - tmp12);

        z10   = CLAMPI(coef[y + 10] - coef[y + 6]);
        z13_1 = mul_669(z10);
        z11   = CLAMPI(coef[y + 2] + coef[y + 14]);
        z13_0 = CLAMPI(coef[y + 10] + coef[y + 6]);
        tmp7 = CLAMPI(z11 + z13_0);
        row_out[y + 0]  = CLAMPI(tmp0 + tmp7);

        z12   = CLAMPI(coef[y + 2] - coef[y + 14]);
        z5 = (mul_473(CLAMPI(z10 + z12)));
        tmp6 = CLAMPI(CLAMPI(z5 - z13_1) - tmp7);
        row_out[y + 2]  = CLAMPI(tmp1 + tmp6);

        tmp10 = CLAMPI(mul_277(z12) - z5);

        tmp5 = CLAMPI(mul_362(CLAMPI(z11 - z13_0)) - tmp6);
        tmp4 = CLAMPI(tmp5 + tmp10);
        row_out[y + 8]  = CLAMPI(tmp3 + tmp4);
        row_out[y + 4]  = CLAMPI(tmp2 + tmp5);
        row_out[y + 6]  = CLAMPI(tmp3 - tmp4);
        row_out[y + 10] = CLAMPI(tmp2 - tmp5);
        row_out[y + 12] = CLAMPI(tmp1 - tmp6);
        row_out[y + 14] = CLAMPI(tmp0 - tmp7);
    }
}

void idct_1d_cols(void) {
    uint8 x;
    int8 z13_0, z13_1;

    for (x = 0; x < 16; x+=2) {

        if (row_out[x + 16] == 0 && row_out[x + 32] == 0 &&
            row_out[x + 48] == 0 && row_out[x + 64] == 0 &&
            row_out[x + 80] == 0 && row_out[x + 96] == 0 &&
            row_out[x + 112] == 0) {

            if (image_size == 0) {
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
            tmp13   = CLAMPI(row_out[x + 32] + row_out[x + 96]);

            tmp10 = CLAMPI(row_out[x + 0]  + row_out[x + 64]);
            tmp0 = CLAMPI(tmp10 + tmp13);
            tmp3 = CLAMPI(tmp10 - tmp13);

            tmp11 = CLAMPI(row_out[x + 0]  - row_out[x + 64]);
            tmp12 = CLAMPI(mul_362(CLAMPI(row_out[x + 32] - row_out[x + 96])) - tmp13);
            tmp1 = CLAMPI(tmp11 + tmp12);
            tmp2 = CLAMPI(tmp11 - tmp12);

            z10     = CLAMPI(row_out[x + 80] - row_out[x + 48]);
            z13_1 = mul_669(z10);

            z11     = CLAMPI(row_out[x + 16] + row_out[x + 112]);
            z13_0   = CLAMPI(row_out[x + 80] + row_out[x + 48]);
            tmp7 = CLAMPI(z11 + z13_0);

            z12     = CLAMPI(row_out[x + 16] - row_out[x + 112]);
            z5 = mul_473(CLAMPI(z10 + z12));
            tmp6 = CLAMPI(CLAMPI(z5 - z13_1) - tmp7);

            tmp10 = CLAMPI(mul_277(z12) - z5);
            tmp5 = CLAMPI(mul_362(CLAMPI(z11 - z13_0)) - tmp6);
            tmp4 = CLAMPI(tmp5 + tmp10);

            if (image_size == 0) {
              idx4[x] = idx4[x + 1] = CLAMPU(tmp3 + tmp4);
              idx0[x] = idx0[x + 1] = CLAMPU(tmp0 + tmp7);
              idx2[x] = idx2[x + 1] = CLAMPU(tmp2 + tmp5);
              idx6[x] = idx6[x + 1] = CLAMPU(tmp1 - tmp6);

              idx1[x] = idx1[x + 1] = CLAMPU(tmp1 + tmp6);
              idx3[x] = idx3[x + 1] = CLAMPU(tmp3 - tmp4);
              idx5[x] = idx5[x + 1] = CLAMPU(tmp2 - tmp5);
              idx7[x] = idx7[x + 1] = CLAMPU(tmp0 - tmp7);
            } else {
              idx2[x/2] = CLAMPU(tmp3 + tmp4);
              idx0[x/2] = CLAMPU(tmp0 + tmp7);
              idx1[x/2] = CLAMPU(tmp2 + tmp5);
              idx3[x/2] = CLAMPU(tmp1 - tmp6);
            }
        }
    }
}

void init_idx(void) {
  idx0 = raw_image;
}

void update_idx(void) {
  idx1 = idx0 + RAW_WIDTH;
  idx2 = idx1 + RAW_WIDTH;
  idx3 = idx2 + RAW_WIDTH;
  idx4 = idx3 + RAW_WIDTH;
  idx5 = idx4 + RAW_WIDTH;
  idx6 = idx5 + RAW_WIDTH;
  idx7 = idx6 + RAW_WIDTH;
}
