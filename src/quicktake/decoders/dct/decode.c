/* 6502-friendly decoder for the Dycam 10-C. Based on the
 * es3000_decode.py included in this directory, which is
 * an LLM-generated reverse-engineer of the Amiga binary
 * (https://aminet.net/package/driver/other/ES3000_Demo)
 * that has been provided to me.
 * 
 * Of course full standard DCT based on doubles was never
 * going to be okay so DCT core replaced by a Loeffler-
 * Ligtenberg-Moschytz implementation. Descaled so 8-bit
 * maths is possible, and image building rewritten to
 * avoid indexes as much as possible.
 *
 * TBD: write it in assembly.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>

extern uint32_t data_size;

uint16_t width, height;

static uint8_t SCAN[64] = {
     0, 1, 8,16, 9, 2, 3,10,
    17,24,32,25,18,11, 4, 5,
    12,19,26,33,40,48,41,34,
    27,20,13, 6, 7,14,21,28,
    35,42,49,56,57,50,43,36,
    29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,
    53,60,61,54,47,55,62,63,
};

static uint8_t normal_bits[64] = {
    8,8,8,7,7,7,7,7,
    7,7,6,5,6,7,7,7,
    6,5,5,5,5,3,0,0,
    5,5,5,6,6,5,4,4,
    0,0,0,0,0,0,0,0,
    4,4,4,0,0,0,0,0,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};

static uint8_t superfine_bits[64] = {
    10,10,10,9,9,9,8,8,
    8,8,7,7,7,8,8,8,
    7,7,7,7,6,5,6,6,
    7,7,7,7,7,7,6,6,
    6,6,5,4,4,4,5,6,
    6,6,6,6,6,6,5,4,
    3,3,3,5,6,6,5,4,
    3,3,3,3,4,3,3,3
};

static uint8_t normal_shift[64] = {
    2,2,2,2,2,2,2,2,
    2,2,2,3,2,2,2,2,
    2,3,3,3,2,3,7,7,
    3,3,3,2,2,3,3,3,
    7,7,6,5,5,5,6,7,
    3,3,3,7,7,7,6,5,
    5,5,5,6,7,7,6,6,
    5,5,5,5,6,5,5,5
};

static uint8_t superfine_shift[64] = {
    0,0,0,0,0,0,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    2,2,2,1,1,1,1,2,
    2,2,2,2,2,2,2,2
};

static uint8_t *bits_table;
static uint8_t *shift_table;

uint8_t *cache, *cur_cache_ptr;
uint8_t *image, *dst;
int nbits_avail = 8;
uint16_t bitmask[16] = {
  0b0000000000000001,
  0b0000000000000010,
  0b0000000000000100,
  0b0000000000001000,
  0b0000000000010000,
  0b0000000000100000,
  0b0000000001000000,
  0b0000000010000000,
  0b0000000100000000,
  0b0000001000000000,
  0b0000010000000000,
  0b0000100000000000,
  0b0001000000000000,
  0b0010000000000000,
  0b0100000000000000,
  0b1000000000000000,
};

uint16_t negate[16] = {
  0b1111111111111111,
  0b1111111111111110,
  0b1111111111111100,
  0b1111111111111000,
  0b1111111111110000,
  0b1111111111100000,
  0b1111111111000000,
  0b1111111110000000,
  0b1111111100000000,
  0b1111111000000000,
  0b1111110000000000,
  0b1111100000000000,
  0b1111000000000000,
  0b1110000000000000,
  0b1100000000000000,
  0b1000000000000000,
};
static const int16_t *g_src;
static int16_t *g_dst;
uint32_t nmults = 0;

#if 0
static const int8_t basis[8][8] =
{
    { 11, 16, 15, 14, 11,  8,  6,  3 },
    { 11, 14,  6, -8,-11,-16,-15, -3 },
    { 11,  8, -6,-16,-11,  3, 15, 14 },
    { 11,  3,-15, -8, 11, 14, -6,-16 },
    { 11, -3,-15,  8, 11,-14, -6, 16 },
    { 11, -8, -6, 16,-11, -3, 15,-14 },
    { 11,-14,  6,  8,-11, 16,-15,  3 },
    { 11,-16, 15,-14, 11, -8,  6, -3 }
};
static void idct_1d_col(void)
{
    for (uint8_t out = 0; out < 8; out++) {

        const int8_t *b = basis[out];
        int16_t s = 0;

        for (uint8_t k = 0; k < 8; k++) {
            s += b[k] * g_src[k * 8];
            nmults++;
        }

        g_dst[out * 8] = s >> 5;
    }
}
static void idct_1d_row(void)
{
    int16_t in[8];

    memcpy(in, g_src, sizeof(in));

    for (uint8_t out = 0; out < 8; out++) {

        const int8_t *b = basis[out];
        int16_t s = 0;

        for (uint8_t k = 0; k < 8; k++) {
            s += b[k] * in[k];
        }

        g_dst[out] = s;
    }
}
#else
static int16_t mul_362(int16_t w)
{
  uint32_t x;
  x = (uint32_t)w * 362;
  x >>= 8;
  if ((int16_t)x < -127) { printf("%d*362 = %d\n", w, x); }
  if ((int16_t)x > 127)  { printf("%d*362 = %d\n", w, x); }
  return (uint16_t)x;
}

static int16_t mul_473(int16_t w)
{
  uint32_t x;
  x = (uint32_t)w * 473;
  x >>= 8;

  if ((int16_t)x < -127) { printf("%d*473 = %d\n", w, x); }
  if ((int16_t)x > 127)  { printf("%d*473 = %d\n", w, x); }
  return (uint16_t)x;
}

static int16_t mul_277(int16_t w)
{
  uint32_t x;
  x = (uint32_t)w * 277;
  x >>= 8;

  if ((int16_t)x < -127) { printf("%d*277 = %d\n", w, x); }
  if ((int16_t)x > 127)  { printf("%d*277 = %d\n", w, x); }
  return (uint16_t)x;
}

static int16_t mul_669(int16_t w)
{
  uint32_t x;
  x = (uint32_t)w * 669;
  x >>= 8;

  if ((int16_t)x < -127) { printf("%d*669 = %d\n", w, x); }
  if ((int16_t)x > 127)  { printf("%d*669 = %d\n", w, x); }
  return (uint16_t)x;
}
#define DESCALE_FACTOR 1

int8_t coef[64];
int8_t row_out[128]; /* Twice as large as needed but simplifies computations. */
static void idct_1d_rows(void)
{
    printf("row idct loop\n");
    for (uint8_t y = 0; y < 8*16; y+=16) {
        const int8_t *x = &coef[y >> 1];

        int8_t tmp0, tmp1, tmp2, tmp3;
        int8_t tmp4, tmp5, tmp6, tmp7;
        int8_t tmp10, tmp11, tmp12, tmp13;
        int8_t z5, z10, z11, z12, z13;

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

        tmp13 = x[2] + x[6];
        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;

        tmp12 = x[2] - x[6];
        if (tmp12) {
          tmp12 = mul_362(tmp12);
        }
        tmp12 -= tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        z13 = x[5] + x[3];
        z11 = x[1] + x[7];
        z12 = x[1] - x[7];

        tmp7 = z11 + z13;

        if (z11 - z13) {
          tmp11 = mul_362(z11 - z13);
          nmults++;
        } else {
          tmp11 = 0;
        }

        z10 = x[5] - x[3];
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

        row_out[y + 0] = (uint8_t)(tmp0 + tmp7);
        row_out[y + 2] = (uint8_t)(tmp1 + tmp6);
        row_out[y + 4] = (uint8_t)(tmp2 + tmp5);
        row_out[y + 6] = (uint8_t)(tmp3 - tmp4);
        row_out[y + 8] = (uint8_t)(tmp3 + tmp4);
        row_out[y + 10] = (uint8_t)(tmp2 - tmp5);
        row_out[y + 12] = (uint8_t)(tmp1 - tmp6);
        row_out[y + 14] = (uint8_t)(tmp0 - tmp7);
    }
}

#define RAW_WIDTH 512
#define DECODE_WIDTH 320
#define DECODE_HEIGHT 240
#define CLAMPU(x) (((uint16_t)(x) << DESCALE_FACTOR) > 255 ? 255 : x<<DESCALE_FACTOR)

static void idct_1d_cols(void)
{
    for (uint8_t x = 0; x < 16; x+=2) {
        int8_t tmp0, tmp1, tmp2, tmp3;
        int8_t tmp4, tmp5, tmp6, tmp7;
        int8_t tmp10, tmp11, tmp12, tmp13;
        int8_t z5, z10, z11, z12, z13;

        if (row_out[x + 16] == 0 && row_out[x + 32] == 0 &&
            row_out[x + 48] == 0 && row_out[x + 64] == 0 &&
            row_out[x + 80] == 0 && row_out[x + 96] == 0 &&
            row_out[x + 112] == 0) {

            if (width == 160) {
              dst[x + 0*RAW_WIDTH] =
                dst[x + 1*RAW_WIDTH] =
                dst[x + 2*RAW_WIDTH] =
                dst[x + 3*RAW_WIDTH] =
                dst[x + 4*RAW_WIDTH] =
                dst[x + 5*RAW_WIDTH] =
                dst[x + 6*RAW_WIDTH] =
                dst[x + 7*RAW_WIDTH] =
              dst[x + 1 + 0*RAW_WIDTH] =
                dst[x + 1 + 1*RAW_WIDTH] =
                dst[x + 1 + 2*RAW_WIDTH] =
                dst[x + 1 + 3*RAW_WIDTH] =
                dst[x + 1 + 4*RAW_WIDTH] =
                dst[x + 1 + 5*RAW_WIDTH] =
                dst[x + 1 + 6*RAW_WIDTH] =
                dst[x + 1 + 7*RAW_WIDTH] = CLAMPU(row_out[x + 0]);
            } else {
              dst[x/2 + 0*RAW_WIDTH] = 
                dst[x/2 + 1*RAW_WIDTH] = 
                dst[x/2 + 2*RAW_WIDTH] = 
                dst[x/2 + 3*RAW_WIDTH] = CLAMPU(row_out[x + 0]);
            }
            
        } else {
            tmp10 = (int8_t)row_out[x + 0] + (int8_t)row_out[x + 64];
            tmp11 = (int8_t)row_out[x + 0] - (int8_t)row_out[x + 64];

            tmp13 = (int8_t)row_out[x + 32] + (int8_t)row_out[x + 96];
            tmp0 = tmp10 + tmp13;
            tmp3 = tmp10 - tmp13;

            tmp12 = (int8_t)row_out[x+32] - (int8_t)row_out[x + 96];
            if (tmp12) {
              tmp12 = mul_362(tmp12);
              nmults++;
            }
            tmp12 -= tmp13;

            tmp1 = tmp11 + tmp12;
            tmp2 = tmp11 - tmp12;

            z13 = (int8_t)row_out[x + 80] + (int8_t)row_out[x + 48];
            z10 = (int8_t)row_out[x + 80] - (int8_t)row_out[x + 48];
            z11 = row_out[x + 16] + (int8_t)row_out[x + 112];
            z12 = row_out[x + 16] - (int8_t)row_out[x + 112];

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


            if (width == 160) {
              dst[x + 0*RAW_WIDTH] = dst[x + 1 + 0*RAW_WIDTH] = CLAMPU(tmp0 + tmp7);
              dst[x + 2*RAW_WIDTH] = dst[x + 1 + 2*RAW_WIDTH] = CLAMPU(tmp2 + tmp5);
              dst[x + 4*RAW_WIDTH] = dst[x + 1 + 4*RAW_WIDTH] = CLAMPU(tmp3 + tmp4);
              dst[x + 6*RAW_WIDTH] = dst[x + 1 + 6*RAW_WIDTH] = CLAMPU(tmp1 - tmp6);
              
              dst[x + 1*RAW_WIDTH] = dst[x + 1 + 1*RAW_WIDTH] = CLAMPU(tmp1 + tmp6);
              dst[x + 3*RAW_WIDTH] = dst[x + 1 + 3*RAW_WIDTH] = CLAMPU(tmp3 - tmp4);
              dst[x + 5*RAW_WIDTH] = dst[x + 1 + 5*RAW_WIDTH] = CLAMPU(tmp2 - tmp5);
              dst[x + 7*RAW_WIDTH] = dst[x + 1 + 7*RAW_WIDTH] = CLAMPU(tmp0 - tmp7);
            } else {
              dst[x/2 + 0*RAW_WIDTH] = CLAMPU(tmp0 + tmp7);
              dst[x/2 + 1*RAW_WIDTH] = CLAMPU(tmp2 + tmp5);
              dst[x/2 + 2*RAW_WIDTH] = CLAMPU(tmp3 + tmp4);
              dst[x/2 + 3*RAW_WIDTH] = CLAMPU(tmp1 - tmp6);
            }
        }
    }
}

#endif

uint16_t total_blocks;
uint8_t blocks_per_row;
static void decode_plane(void)
{
    uint8_t blocks_rem_in_row = blocks_per_row;

    dst = image;

    for (int block = 0; block < total_blocks; block++) {
        printf("block %d\n", block);
        for (int scan = 0; scan < 64; scan++) {
            uint8_t r = SCAN[scan];
            uint8_t b = bits_table[scan];
            uint8_t ob, neg = 0;
            uint8_t bitpos = 0;
            int16_t x = 0;
            
            if (!b) {
                coef[r] = 0;
                continue;
            }

            ob = b + bitpos;

            bitpos = shift_table[scan];

            while (b--) {
                if (nbits_avail == 0) {
                    p++;
                    nbits_avail = 8;
                }

                neg = *p & 1;
                *p >>= 1;
                if (neg) {
                  x |= bitmask[bitpos];
                }
                bitpos++;
                nbits_avail--;
            }

            /* extend sign bit */
            if (scan && neg) {
                x = (uint16_t)x | negate[ob];
            }
            coef[r] = (int8_t)(x >> (DESCALE_FACTOR+2));
        }

        idct_1d_rows();
        idct_1d_cols();

        if (--blocks_rem_in_row == 0) {
          uint8_t *old_dst = dst;
          if (width == 160) {
            dst += 8*RAW_WIDTH-DECODE_WIDTH+16;
          } else {
            dst += 4*RAW_WIDTH-DECODE_WIDTH+8;
          }
          blocks_rem_in_row = blocks_per_row;
        } else {
          dst += width == 160 ? 16 : 8;
        }
        printf("Incr dst, now %d from image\n", dst-image);

        /* Each block consumes a constant number of full bytes */
        assert(nbits_avail == 0);
    }
    printf("total mults %d\n", nmults);
}
/* ---------- SAVE PGM ---------- */

static int save_pgm(
    const char *path,
    uint8_t *img,
    int width,
    int height)
{
    FILE *f = fopen(path, "wb");

    if (!f)
        return 0;

    fprintf(f, "P5\n%d %d\n255\n",
            width, height);
    
    p = img;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width ; x++) {

            uint8_t v = p[x];

            fwrite(&v, 1, 1, f);
        }
        p += RAW_WIDTH;
    }
    fclose(f);
    return 1;
}

/* ---------- MAIN DECODE ---------- */

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr,
                "usage: %s file.cdcx\n",
                argv[0]);
        return 1;
    }

    for (int arg = 1; arg < argc; arg++) {

        const char *path = argv[arg];

        FILE *f = fopen(path, "rb");

        if (!f) {
            perror(path);
            continue;
        }

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);

        uint8_t *raw = malloc(size);

        fread(raw, 1, size, f);
        fclose(f);

        if (memcmp(raw, "Cdcx", 4) != 0) {
            fprintf(stderr,
                    "%s: not a Cdcx file\n",
                    path);
            free(raw);
            continue;
        }

        bits_table = normal_bits;
        shift_table = normal_shift;
        width = 320;

        switch (data_size) {
        case 24000:
          blocks_per_row = 20;
          width = 160;
          total_blocks = 600;
          break;
        case 192000:
          bits_table = superfine_bits;
          shift_table = superfine_shift;
          /* Fallthrough */
        case 96000:
          blocks_per_row = 40;
          total_blocks = 2400;
          break;
        default:
          printf("Invalid file.\n");
          exit(1);
        }

        decode_plane();

        char outname[1024];
        snprintf(outname,
                 sizeof(outname),
                 "%s.pgm",
                 path);

        save_pgm(outname,
                 image,
                 DECODE_WIDTH,
                 DECODE_HEIGHT);

        free(image);
        free(raw);
    }

    return 0;
}
