#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "progress_bar.h"
#include "qt-conv.h"

#pragma inline-stdfuncs(push, on)
#pragma allow-eager-inline(push, on)
#pragma codesize(push, 200)
#pragma register-vars(push, on)

/* bitbuff state at end of first loop */
static uint32 prev_bitbuf_a = 0;
static uint8 prev_vbits_a = 0;
static uint32 prev_offset_a = 0;

char magic[5] = QT100_MAGIC;
char *model = "100";
uint16 raw_width = 640;
uint16 raw_image_size = (QT_BAND) * 640;
uint8 raw_image[QT_BAND * 640];
uint16 cache_size = 4096;
uint8 cache[4096];

static uint8 h_plus1, h_plus2, h_plus4;
static uint16 width_plus2;
static uint16 pgbar_state;
static uint16 threepxband_size;

#define PIX_WIDTH 644
static uint8 pixel[(QT_BAND+5)*PIX_WIDTH];
#define PIX(row,col) pixel[width*(row)+(col)]
#define PIX_IDX(row,col) (width*(row)+(col))
#define PIX_DIRECT_IDX(idx) pixel[idx]

void qt_load_raw(uint16 top, uint8 h)
{
  static const short gstep[16] =
  { -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
  int16 val = 0;
  uint8 row;
  register uint8 *dst;
  register uint16 col, idx;
  uint8 *src;
  uint16 idx_rowplus1, idx_rowminus1;

  if (top == 0) {
    getbits(0);
    h_plus1 = h + 1;
    h_plus2 = h + 2;
    h_plus4 = h + 4;
    width_plus2 = width + 2;
    pgbar_state = 0;
    threepxband_size = 3*width;
    memset (pixel, 0x80, sizeof pixel);
  } else {
    memcpy(pixel, pixel + PIX_IDX(QT_BAND + 2, 0), threepxband_size);
    memset (pixel + PIX_IDX(3, 0), 0x80, sizeof pixel - threepxband_size);
    bitbuf = prev_bitbuf_a;
    vbits = prev_vbits_a;
    iseek(prev_offset_a);
  }

  for (row=2; row < h_plus4; row++) {
    if (row < h_plus2)
      progress_bar(-1, -1, 80*22, pgbar_state++, height);
    col = 2+(row & 1);
    idx = PIX_IDX(row, col);
    idx_rowminus1 = idx - width;
    idx_rowplus1 = idx + width;
    for (; col < width_plus2; col+=2) {
      val = ((PIX_DIRECT_IDX(idx_rowminus1 - 1) // row-1,col-1
              + 2*PIX_DIRECT_IDX(idx_rowminus1 + 1) //row-1,col+1
              + PIX_DIRECT_IDX(idx - 2)) >> 2) //row,col-2
             + gstep[getbits(4)];

      if (val < 0)
        val = 0;
      if (val > 255)
        val = 255;
      PIX_DIRECT_IDX(idx) = val;

      if (col < 4){
        /* row, col-2 */
        PIX_DIRECT_IDX(idx - 2) = PIX_DIRECT_IDX(idx_rowplus1 + (~row & 1)) = val;
        idx_rowplus1 += 2; /* No need to follow this index after col >= 4 */
      }
      if (row == 2 && top == 0){
        /* row-1,col+1 / row-1,col+3*/
        PIX_DIRECT_IDX(idx_rowminus1+1) = PIX_DIRECT_IDX(idx_rowminus1+3) = val;
      }
      idx += 2;
      idx_rowminus1 += 2;
    }

    PIX_DIRECT_IDX(idx) = val;

    if(row == h_plus1) {
      /* Save state at end of first loop */
      prev_bitbuf_a = bitbuf;
      prev_vbits_a = vbits;
      prev_offset_a = cache_read_since_inval();
    }
  }

  for (row=2; row < h_plus2; row++) {
    col = 3-(row & 1);
    idx = PIX_IDX(row, col);
    for (; col < width_plus2; col+=2) {
      val = ((PIX_DIRECT_IDX(idx-1) // row,col-1
            + (PIX_DIRECT_IDX(idx) << 2) //row,col
            +  PIX_DIRECT_IDX(idx+1)) >> 1) //row,col+1
            - 0x100;

      if (val < 0)
        val = 0;
      if (val > 255)
        val = 255;
      PIX_DIRECT_IDX(idx) = val;
      idx += 2;
    }
  }

  dst = raw_image;
  src = pixel + PIX_IDX(2, 2);
  for (row=0; row < h; row++) {
    for (col=0; col < width; col++) {
      *dst = *src;
      dst++;
      src++;
    }
  }
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
