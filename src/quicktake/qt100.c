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
static uint16 threepxband_size, work_size;
static uint16 band_size;
static uint8 *last_two_lines, *third_line;
static uint8 at_very_first_line;

#define PIX_WIDTH 644
static uint8 pixel[(QT_BAND+5)*PIX_WIDTH];
#define PIX(row,col) pixel[width*(row)+(col)]
#define PIX_IDX(row,col) (width*(row)+(col))
#define PIX_DIRECT_IDX(idx) pixel[idx]

void qt_load_raw(uint16 top, uint8 h)
{
  static const short gstep[16] =
  { -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
  static int16 val;
  static uint8 val_col_minus2;
  static uint8 row;
  static uint8 *src;
  static uint8 *idx_forward, *idx_behind;
  register uint8 *dst, *idx;
  register uint16 col;

  if (top == 0) {
    getbits(0);
    at_very_first_line = 1;
    h_plus1 = h + 1;
    h_plus2 = h + 2;
    h_plus4 = h + 4;
    width_plus2 = width + 2;
    pgbar_state = 0;
    band_size = h * width;
    /* calculate offsets to shift the last lines */
    threepxband_size = 3*width;
    work_size = sizeof pixel - threepxband_size;
    last_two_lines = pixel + PIX_IDX(QT_BAND + 2, 0);
    third_line = pixel + PIX_IDX(3, 0);
    memset (pixel, 0x80, sizeof pixel);
  } else {
    /* Shift last pixel lines to start */
    memcpy(pixel, last_two_lines, threepxband_size);
    /* And reset the others */
    memset (third_line, 0x80, work_size);

    /* Reset bitbuf where it should be */
    bitbuf = prev_bitbuf_a;
    vbits = prev_vbits_a;
    iseek(prev_offset_a);
  }

  for (row=2; row < h_plus4; row++) {
    if (row < h_plus2) {
      pgbar_state++;
      progress_bar(-1, -1, 80*22, pgbar_state, height);
    }

    col = 2+(row & 1);
    idx = pixel + PIX_IDX(row, col);
    idx_behind = idx - width - 1;
    idx_forward = idx + width;
    
    val_col_minus2 = *(idx - 2);
    for (; col < width_plus2; col+=2) {
      val = ((*(idx_behind)           // row-1, col-1
              + (*(idx_behind + 2))*2 // row-1, col+1
              + val_col_minus2) >> 2) // row  , col-2
             + gstep[getbits(4)];

      if (val < 0)
        *(idx) = 0;
      else if (val > 255)
        *(idx) = 255;
      else
        *(idx) = val;

      /* Cache it for next loop before shifting */
      val_col_minus2 = val;

      if (col < 4) {
        /* row, col-2 */
        *(idx - 2) = *(idx_forward + (~row & 1)) = val;
        idx_forward += 2;
      }

      idx += 2;
      idx_behind += 2;

      if (at_very_first_line) {
        /* row-1,col+1 / row-1,col+3*/
        *(idx_behind) = *(idx_behind + 2) = val;
      }
    }

    *(idx) = val;

    if(row == h_plus1) {
      /* Save bitbuf state at end of first loop */
      prev_bitbuf_a = bitbuf;
      prev_vbits_a = vbits;
      prev_offset_a = cache_read_since_inval();
    }
    at_very_first_line = 0;
  }

  for (row = 2; row < h_plus2; row++) {
    col = 3 - (row & 1);
    idx = pixel + PIX_IDX(row, col);

    for (; col < width_plus2; col+=2) {
      val = ((*(idx-1) // row,col-1
            + (*(idx) << 2) //row,col
            +  *(idx+1)) >> 1) //row,col+1
            - 0x100;

      if (val < 0)
        *(idx) = 0;
      else if (val > 255)
        *(idx) = 255;
      else
        *(idx) = val;

      idx += 2;
    }
  }

  /* Move from tmp pixel array to raw_image */
  src = pixel + PIX_IDX(2, 2);
  dst = raw_image;
  memcpy(dst, src, band_size);
}

#pragma register-vars(pop)
#pragma codesize(pop)
#pragma allow-eager-inline(pop)
#pragma inline-stdfuncs(pop)
