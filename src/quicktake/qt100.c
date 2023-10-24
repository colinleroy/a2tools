#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "qt-conv.h"


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

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))

static uint8 h_plus2, h_plus4;
static uint16 width_plus2;

#define PIX_WIDTH 644
static uint8 pixel[(QT_BAND+5)*PIX_WIDTH];
#define PIX(row,col) pixel[(row)*PIX_WIDTH+(col)]
#define PIX_IDX(row,col) ((row)*PIX_WIDTH+(col))
#define PIX_DIRECT_IDX(idx) pixel[idx]

void qt_load_raw(uint16 top, uint8 h)
{
  static const short gstep[16] =
  { -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
  int16 val = 0;
  uint8 row;
  uint16 col, idx, idx2;

  if (top == 0) {
    getbits(0);
    h_plus2 = h + 2;
    h_plus4 = h + 4;
    width_plus2 = width + 2;
    memset (pixel, 0x80, sizeof pixel);
  } else {
    memcpy(pixel, pixel + PIX_IDX(QT_BAND + 2, 0), 3*PIX_WIDTH);
    memset (pixel + PIX_IDX(3, 0), 0x80, sizeof pixel - (3*PIX_WIDTH));
    bitbuf = prev_bitbuf_a;
    vbits = prev_vbits_a;
    iseek(prev_offset_a);
  }

  for (row=2; row < h_plus4; row++) {
    printf(".");
    col = 2+(row & 1);
    idx = PIX_IDX(row, col);
    for (; col < width_plus2; col+=2) {
      val = ((PIX_DIRECT_IDX(idx - PIX_WIDTH - 1) // row-1,col-1
              + 2*PIX_DIRECT_IDX(idx - PIX_WIDTH + 1) //row-1,col+1
              + PIX_DIRECT_IDX(idx - 2)) >> 2) //row,col-2
             + gstep[getbits(4)];

      PIX_DIRECT_IDX(idx) = val = LIM(val,0,255);

      if (col < 4){
        /* row, col-2*/
        PIX_DIRECT_IDX(idx - 2) = PIX(row+1,~row & 1) = val;
      }
      if (row == 2 && top == 0){
        /* row-1,col+1 / row-1,col+3*/
        PIX_DIRECT_IDX(idx-PIX_WIDTH+1) = PIX_DIRECT_IDX(idx-PIX_WIDTH+3) = val;
      }
      idx += 2;
    }

    PIX_DIRECT_IDX(idx) = val;

    if(row == h_plus2) {
      /* Save state at end of first loop */
      prev_bitbuf_a = bitbuf;
      prev_vbits_a = vbits;
      prev_offset_a = cache_read_since_inval();
    }
  }

  for (row=2; row < h_plus2; row++) {
    col = 3-(row & 1);
    idx = PIX_IDX(row,col);
    for (; col < width_plus2; col+=2) {
      val = ((PIX_DIRECT_IDX(idx-1) // row,col-1
            + (PIX_DIRECT_IDX(idx) << 2) //row,col
            +  PIX_DIRECT_IDX(idx+1)) >> 1) //row,col+1
            - 0x100;
      PIX_DIRECT_IDX(idx) = LIM(val,0,255);
      idx += 2;
    }
  }
  for (row=0; row < h; row++) {
    idx = RAW_IDX(row,0);
    idx2 = PIX_IDX(row+2, 2);
    for (col=0; col < width; col++) {
      RAW_DIRECT_IDX(idx) = PIX_DIRECT_IDX(idx2);
      idx++;
      idx2++;
    }
  }
}
