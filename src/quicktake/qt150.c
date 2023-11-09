/*
   QTKN (QuickTake 150/200) decoding algorithm
   Copyright 2023, Colin Leroy-Mira <colin@colino.net>

   Heavily based on dcraw.c -- Dave Coffin's raw photo decoder
   Copyright 1997-2018 by Dave Coffin, dcoffin a cybercom o net

   Welcome to pointer arithmetic hell - you can refer to dcraw's
   kodak_radc_load_raw() is you prefer triple-dimensional array hell
 */

/* Handle pic by horizontal bands for memory constraints reasons.
 * Bands need to be a multiple of 4px high for compression reasons
 * on QT 150/200 pictures,
 * and a multiple of 5px for nearest-neighbor scaling reasons.
 * (480 => 192 = *0.4, 240 => 192 = *0.8)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "progress_bar.h"
#include "qt-conv.h"

#define radc_token(tree) ((int8) getbithuff(8,huff[tree]))

/* Shared with qt-conv.c */
char magic[5] = QT150_MAGIC;
char *model = "150/200";
uint16 cache_size = 1536;
uint8 cache[1536];

static uint16 val_from_last[256];
static uint16 huff[19][256];
static int16 x, s, i, tree, tmp_i16;
static uint16 c, half_width, col;
static uint8 r, nreps, rep, row, y, mul[3], t;
#define DATABUF_SIZE 386
static uint16 buf[3][3][DATABUF_SIZE], (*cur_buf)[DATABUF_SIZE], *cur_buf_y, *cur_buf_prevy;
static uint16 *cur_buf_x, *cur_buf_x_plus1;
static uint16 *midbuf1, *midbuf2;
static uint16 val;
static int8 tk;
static uint8 tmp8, not_c;
static uint16 tmp16, tmp16_2;
static uint32 tmp32;
static uint8 *raw_ptr1, *raw_ptr2;
static uint16 row_idx, row_idx_plus2, row_idx_shift;

static const int8 src[] = {
  1,1, 2,3, 3,4, 4,2, 5,7, 6,5, 7,6, 7,8,
  1,0, 2,1, 3,3, 4,4, 5,2, 6,7, 7,6, 8,5, 8,8,
  2,1, 2,3, 3,0, 3,2, 3,4, 4,6, 5,5, 6,7, 6,8,
  2,0, 2,1, 2,3, 3,2, 4,4, 5,6, 6,7, 7,5, 7,8,
  2,1, 2,4, 3,0, 3,2, 3,3, 4,7, 5,5, 6,6, 6,8,
  2,3, 3,1, 3,2, 3,4, 3,5, 3,6, 4,7, 5,0, 5,8,
  2,3, 2,6, 3,0, 3,1, 4,4, 4,5, 4,7, 5,2, 5,8,
  2,4, 2,7, 3,3, 3,6, 4,1, 4,2, 4,5, 5,0, 5,8,
  2,6, 3,1, 3,3, 3,5, 3,7, 3,8, 4,0, 5,2, 5,4,
  2,0, 2,1, 3,2, 3,3, 4,4, 4,5, 5,6, 5,7, 4,8,
  1,0, 2,2, 2,-2,
  1,-3, 1,3,
  2,-17, 2,-5, 2,5, 2,17,
  2,-7, 2,2, 2,9, 2,18,
  2,-18, 2,-9, 2,-2, 2,7,
  2,-28, 2,28, 3,-49, 3,-9, 3,9, 4,49, 5,-79, 5,79,
  2,-1, 2,13, 2,26, 3,39, 4,-16, 5,55, 6,-37, 6,76,
  2,-26, 2,-13, 2,1, 3,-39, 4,16, 5,-55, 6,-76, 6,37
};
static uint8 last[3] = { 16,16,16 };

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)


void qt_load_raw(uint16 top, uint8 h)
{
  if (top == 0) {
    /* Init */
    for (s = i = 0; i < sizeof src; i += 2) {
      for (c=0; c < 256 >> src[i]; c++) {
        ((uint16 *)(huff))[s] = (src[i] << 8 | (uint8) src[i+1]);
        s++;
      }
    }

    for (c=0; c < 256; c++) {
      huff[18][c] = (1284 | c);
    }
    getbits(0);

    for (i=0; i < sizeof(buf)/sizeof(uint16); i++) {
      ((uint16 *)buf)[i] = 2048;
    }

    for (i = 1; i < 256; i++) {
      tmp32 = (0x1000000L/(uint32)i);
      FAST_SHIFT_RIGHT_8_LONG(tmp32);
      val_from_last[i] = (tmp32 >> 4);
    }

    half_width = width / 2;
    row_idx_shift = width * 4;
  }

  row_idx = 0;
  row_idx_plus2 = width * 2;

  for (row=0; row < h; row+=4) {
    progress_bar(-1, -1, 80*22, (top + row), height);
    mul[0] = getbits(6);
    mul[1] = getbits(6);
    mul[2] = getbits(6);

    cur_buf = buf[0];
    for (c=0; c < 3; c++) {
      if (c)
        not_c = 0;
      else
        not_c = 1;

      t = mul[c];

      val = val_from_last[last[c]];
      val *= t;

      for (i=0; i < sizeof(buf[0])/sizeof(uint16); i++) {
        tmp32 = (uint32)(((uint16 *)cur_buf)[i] * (uint32)val);
        /* Shift >> 12 */
        FAST_SHIFT_RIGHT_8_LONG(tmp32);
        ((uint16 *)cur_buf)[i] = tmp32 >> 4;
      }

      last[c] = t;
      midbuf1 = &(cur_buf[1][half_width]);
      midbuf2 = &(cur_buf[2][half_width]);

      for (r=0; r <= not_c; r++) {
        tree = t << 7;
        *midbuf1 = tree;
        *midbuf2 = tree;

        for (tree = 1, col = half_width; col; ) {
          if ((tree = (int16)radc_token(tree))) {
            col -= 2;
            if (tree == 8) {
              x = col + 1;
              cur_buf_x = cur_buf[1] + x;
              for (y=1; y < 3; y++) {
                *cur_buf_x = (uint8) radc_token(18) * t;
                cur_buf_x--;
                *cur_buf_x = (uint8) radc_token(18) * t;
                cur_buf_x += DATABUF_SIZE + 1;
              }
            } else {
              tmp16 = col + 1;
              tmp_i16 = tree + 10;
              cur_buf_prevy = cur_buf[0] + tmp16;
              cur_buf_x = cur_buf_prevy + DATABUF_SIZE;
              cur_buf_x_plus1 = cur_buf_x + 1;
              for (y=1; ; y++) {
                /* Unrolled */
                tk = radc_token(tmp_i16);
                *cur_buf_x = tk * 16;

                if (c) {
                  tmp16_2 = (*cur_buf_prevy + *cur_buf_x_plus1);
                  tmp16_2 >>= 1;
                  *cur_buf_x += tmp16_2;
                } else {
                  tmp16_2 = *cur_buf_prevy << 1;
                  tmp16_2 += *(cur_buf_prevy + 1);
                  tmp16_2 += *cur_buf_x_plus1;
                  tmp16_2 >>= 2;
                  *cur_buf_x += tmp16_2;
                }
                cur_buf_x_plus1 = cur_buf_x;
                cur_buf_x--;
                cur_buf_prevy--;
                /* Second with col - 1*/
                tk = radc_token(tmp_i16);
                *cur_buf_x = tk * 16;
                if (c) {
                  tmp16_2 = (*cur_buf_prevy + *cur_buf_x_plus1);
                  tmp16_2 >>= 1;
                  *cur_buf_x += tmp16_2;
                } else {
                  tmp16_2 = *cur_buf_prevy << 1;
                  tmp16_2 += *(cur_buf_prevy + 1);
                  tmp16_2 += *cur_buf_x_plus1;
                  tmp16_2 >>= 2;
                  *cur_buf_x += tmp16_2;
                }
                if (y == 2)
                  break;
                cur_buf_x += DATABUF_SIZE+1;
                cur_buf_x_plus1 += DATABUF_SIZE+1;
                cur_buf_prevy += DATABUF_SIZE+1;
              }
            }
          } else {
            do {
              nreps = (col > 2) ? radc_token(9) + 1 : 1;
              for (rep=0; rep < 8 && rep < nreps && col; rep++) {
                col -= 2;
                if (rep & 1) {
                  /* need to get that, but ignore it */
                  tk = radc_token(10);
                }
                if (c == 0) {
                  tmp16 = col + 1;
                  cur_buf_prevy = cur_buf[0] + tmp16;
                  cur_buf_x = cur_buf_prevy + DATABUF_SIZE;
                  cur_buf_x_plus1 = cur_buf_x + 1;
                  for (y=1; ; y++) {
                    /* Unrolled */
                    if (c) {
                      *cur_buf_x = (*cur_buf_prevy + *(cur_buf_x + 1)) / 2;
                    } else {
                      *cur_buf_x = (*(cur_buf_prevy + 1) + 2*(*cur_buf_prevy) + *(cur_buf_x_plus1)) / 4;
                    }
                    cur_buf_x_plus1 = cur_buf_x;
                    cur_buf_x--;
                    cur_buf_prevy--;
                    /* Second */
                    if (c) {
                      *cur_buf_x = (*cur_buf_prevy + *(cur_buf_x + 1)) / 2;
                    } else {
                      *cur_buf_x = (*(cur_buf_prevy + 1) + 2*(*cur_buf_prevy) + *(cur_buf_x_plus1)) / 4;
                    }
                    if (y == 2)
                      break;
                    cur_buf_x += DATABUF_SIZE+1;
                    cur_buf_x_plus1 += DATABUF_SIZE+1;
                    cur_buf_prevy += DATABUF_SIZE+1;
                  }
                }
              }
            } while (nreps == 9);
          }
        }
        if (c == 0) {
          raw_ptr1 = raw_image + row_idx; //FILE_IDX(row, 0);
          raw_ptr2 = raw_image + row_idx_plus2; //FILE_IDX(row + 2, 0);
          cur_buf_y = cur_buf[1];
          for (y=1; ; y++) {
            cur_buf_x = cur_buf_y;
            for (x=0; x < half_width; x++) {
              val = (*cur_buf_x) / t;
              cur_buf_x++;
              if (val > 255)
                val = 255;
              if (r) {
                *raw_ptr2 = val;
              } else {
                *raw_ptr1 = val;
              }
              raw_ptr1 += 2;
              raw_ptr2 += 2;
            }
            if (y == 2)
              break;
            raw_ptr1++;
            raw_ptr2++;
            cur_buf_y += DATABUF_SIZE;
          }
          memcpy (cur_buf[0]+1, cur_buf[2], sizeof cur_buf[0]-2);
        }
      }
      cur_buf += 3;
    }

    raw_ptr1 = raw_image + row_idx - 1;
    for (y = 4; y; y--) {
      tmp8 = y & 1;
      if (!tmp8) {
        raw_ptr1++;
        x = 0;
      } else {
        x = 1;
      }
      for (; x < width; x+=2) {
        *(raw_ptr1 + 1) = *raw_ptr1;
        raw_ptr1+=2;
      }
      if (!tmp8) {
        raw_ptr1--;
      }
    }

    row_idx += row_idx_shift;
    row_idx_plus2 += row_idx_shift;
  }
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
