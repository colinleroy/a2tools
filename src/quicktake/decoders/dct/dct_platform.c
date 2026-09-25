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
  for (scan = 0; scan < 64; scan++) {
      uint8 r = SCAN[scan];

      if (!(numbits = bits_table[scan])) {
          coef[r] = 0;
          continue;
      }
      ign_bits = shift_table[scan];

      bitpos = 0;

      get_bitval();
      coef[r] = (int8)(bitval);
  }
}

void get_bitval(void) {
  bitval = valneg = 0;

  do {
    if (--nbits_avail < 0) {
        nbits_avail = 7;
        cur_cache_ptr++;
        if (cur_cache_ptr == cache_end) {
            read(ifd, cur_cache_ptr = cache, CACHE_SIZE);
        }
    }

    valneg = *cur_cache_ptr & 1;
    *cur_cache_ptr >>= 1;
    if (--ign_bits >= 0) {
      continue;
    }
    if (valneg) {
      bitval |= bitmask[bitpos];
    }
    bitpos++;
  } while (--numbits);

  /* extend sign bit */
  if (scan && valneg) {
      bitval = (uint16)bitval | negate[bitpos];
  }
}

void advance_block(void) {
  if (--blocks_rem_in_row == 0) {
    if (actual_width == 160) {
      idx0 += 8*RAW_WIDTH-DECODE_WIDTH+16;
    } else {
      idx0 += 4*RAW_WIDTH-DECODE_WIDTH+8;
    }
    blocks_rem_in_row = blocks_per_row;
  } else {
    idx0 += actual_width == 160 ? 16 : 8;
  }
  update_idx();
}

void idct_common(void) {
        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;

        tmp12 = mul_362(tmp12);
        tmp12 -= tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        tmp7 = z11 + z13;

        tmp11 = mul_362(z11 - z13);

        z13 = mul_669(z10);

        z5 = mul_473(z10 + z12);
        tmp12 = z5 - z13;

        tmp10 = mul_277(z12);
        tmp10 -= z5;

        tmp6 = tmp12 - tmp7;
        tmp5 = tmp11 - tmp6;
        tmp4 = tmp5 + tmp10;
}

void idct_1d_rows(void) {
    uint8 y;

    for (y = 0; y < 8*16; y+=16) {
        // const int8 *x = &coef[y];

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

        tmp10 = coef[y + 0] + coef[y + 8];
        tmp11 = coef[y + 0] - coef[y + 8];
        tmp12 = coef[y + 4] - coef[y + 12];
        tmp13 = coef[y + 4] + coef[y + 12];
        z10   = coef[y + 10] - coef[y + 6];
        z11   = coef[y + 2] + coef[y + 14];
        z12   = coef[y + 2] - coef[y + 14];
        z13   = coef[y + 10] + coef[y + 6];

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


#define CLAMPU(x) (((uint8)(x) & 0x80) != 0 ? 255 : ((x)<<DESCALE_FACTOR))
void idct_1d_cols(void) {
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
              idx2[x] = idx2[x + 1] = CLAMPU(tmp2 + tmp5);
              idx4[x] = idx4[x + 1] = CLAMPU(tmp3 + tmp4);
              idx6[x] = idx6[x + 1] = CLAMPU(tmp1 - tmp6);

              idx1[x] = idx1[x + 1] = CLAMPU(tmp1 + tmp6);
              idx3[x] = idx3[x + 1] = CLAMPU(tmp3 - tmp4);
              idx5[x] = idx5[x + 1] = CLAMPU(tmp2 - tmp5);
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
