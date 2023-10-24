#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "qt-conv.h"


char *cache[2];
uint16 cache_size = 4096;

void alloc_cache(void) {
  cache[CACHE_A] = malloc(cache_size);
  cache[CACHE_B] = malloc(cache_size);
  if (cache[CACHE_A] == NULL || cache[CACHE_B] == NULL) {
    printf("Cannot allocate memory");
    exit(1);
  }
}
/* bitbuff state at end of first loop */
static uint32 prev_bitbuf_a = 0;
static uint8 prev_vbits_a = 0;
static uint32 prev_offset_a = 0;

/* bitbuff state at end of full first pass */
static uint32 end_bitbuf_a = 0;
static uint8 end_vbits_a = 0;

/* bitbuff state at end of second loop */
static uint32 prev_bitbuf_b = 0;
static uint8 prev_vbits_b = 0;
static uint32 prev_offset_b = 0;

char magic[5] = QT100_MAGIC;
char *model = "100";
uint16 raw_width = 644;
uint16 raw_image_size = (QT_BAND + 4) * 644;
uint8 raw_image[(QT_BAND + 4) * 644];

#define ABS(x) (x < 0 ? -x : x)
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define SWAP(a,b) { a=a+b; b=a-b; a=a-b; }

void qt_load_raw(uint16 top, uint8 h)
{
  static const int8 gstep[16] =
  { -89,-60,-44,-32,-22,-15,-8,-2,2,8,15,22,32,44,60,89 };
  static const int8 rstep[6][4] =
  { {  -3,-1,1,3  }, {  -5,-1,1,5  }, {  -8,-2,2,8  },
    { -13,-3,3,13 }, { -19,-4,4,19 }, { -28,-6,6,28 } };
  static const uint16 curve[256] =
  { 0,1,2,3,4,5,6,7,8,9,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,
    28,29,30,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,53,
    54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,74,75,76,77,78,
    79,80,81,82,83,84,86,88,90,92,94,97,99,101,103,105,107,110,112,114,116,
    118,120,123,125,127,129,131,134,136,138,140,142,144,147,149,151,153,155,
    158,160,162,164,166,168,171,173,175,177,179,181,184,186,188,190,192,195,
    197,199,201,203,205,208,210,212,214,216,218,221,223,226,230,235,239,244,
    248,252,257,261,265,270,274,278,283,287,291,296,300,305,309,313,318,322,
    326,331,335,339,344,348,352,357,361,365,370,374,379,383,387,392,396,400,
    405,409,413,418,422,426,431,435,440,444,448,453,457,461,466,470,474,479,
    483,487,492,496,500,508,519,531,542,553,564,575,587,598,609,620,631,643,
    654,665,676,687,698,710,721,732,743,754,766,777,788,799,810,822,833,844,
    855,866,878,889,900,911,922,933,945,956,967,978,989,1001,1012,1023 };
  int16 val=0;
  uint8 sharp, rb, row;
  uint16 col, tmp_i16;

  /* Cycle-sparing caches */
  static uint8 h_plus2;
  static uint16 height_plus2, width_plus2, double_raw_width;
  static uint8 v_row_min2_col, v_row_col_min2, v_row_min2_col_min2;
  static uint16 idx, end_idx;

  getbits(0);
  if (top == 0) {
    memset (raw_image, 0x80, sizeof raw_image);
    h_plus2 = h + 2;
    width_plus2 = width+2;
    double_raw_width = raw_width+raw_width;
  } else {
    memcpy(raw_image, raw_image + ((QT_BAND + 2)*644), 2*644);
    bitbuf = prev_bitbuf_a;
    vbits = prev_vbits_a;
    set_cache(prev_offset_a, CACHE_A);
  }

  for (row = 2; row < h_plus2; row++) {
    col=2 + (row & 1);
    idx = RAW_IDX(row,col);
    for (; col < width_plus2; col+=2) {
      val = ((RAW_DIRECT_IDX(idx-raw_width-1) + 2*RAW_DIRECT_IDX(idx-raw_width+1) +
                RAW_DIRECT_IDX(idx-2)) >> 2) + gstep[getbits(4)];
      RAW_DIRECT_IDX(idx) = val = LIM(val,0,255);
      if (col < 4) {
        RAW_DIRECT_IDX(idx-2) = RAW(row+1,~row & 1) = val;
      }
      if (row == 2 && top == 0) {
        RAW_DIRECT_IDX(idx-raw_width+1) = RAW_DIRECT_IDX(idx-raw_width+3) = val;
      }
      idx += 2;
    }
    RAW_DIRECT_IDX(idx) = val;
  }

  /* Save state at end of first loop */
  prev_bitbuf_a = bitbuf;
  prev_vbits_a = vbits;
  prev_offset_a = ftell(ifp);

  /* Save bitbuff state at end of first full pass */
  if (top == 0) {
    height_plus2 = height + 2;
    printf(" (Finish first pass bitbuff...)");
    /* Use an i16 because height can be == 480 */
    for (tmp_i16 = row; tmp_i16 < height_plus2; tmp_i16++) {
      for (col=2+(tmp_i16 & 1); col < width_plus2; col+=2) {
        getbits(4);
      }
    }
    end_bitbuf_a = bitbuf;
    end_vbits_a = vbits;
  } else {
    bitbuf = end_bitbuf_a;
    vbits = end_vbits_a;
  }

  /* Reinit bitbuf for second loop */
  if (prev_offset_b != 0) {
    bitbuf = prev_bitbuf_b;
    vbits = prev_vbits_b;
    set_cache(prev_offset_b, CACHE_B);
  }

  for (rb=0; rb < 2; rb++) {
    /* Row from 2 to 21, then 3 to 22
     * accessing pixels from row-2 (0) to row+2 (24)
     */
    for (row=2+rb; row < h_plus2; row+=2) {
      printf(".");
      col = 3-(row & 1);
      idx = RAW_IDX(row,col);
      for (; col < width_plus2; col+=2) {
        v_row_min2_col = RAW_DIRECT_IDX(idx-double_raw_width); // (row-2,col)
        v_row_col_min2 = RAW_DIRECT_IDX(idx-2); // (row,col-2)

        if ((row < 4 && top == 0) || col < 4) {
          sharp = 2;
        } else {
          v_row_min2_col_min2 = RAW_DIRECT_IDX(idx-double_raw_width-2); // (row-2,col-2)
          val = ABS(v_row_min2_col - v_row_col_min2)
              + ABS(v_row_min2_col - v_row_min2_col_min2)
              + ABS(v_row_col_min2 - v_row_min2_col_min2);
          sharp = val <  4 ? 0 : val <  8 ? 1 : val < 16 ? 2 :
                  val < 32 ? 3 : val < 48 ? 4 : 5;
        }
        val = ((v_row_min2_col + v_row_col_min2) >> 1)
              + rstep[sharp][getbits(2)];
        RAW_DIRECT_IDX(idx) = val = LIM(val,0,255);
        if (row < 4 && top == 0) {
          RAW_DIRECT_IDX(idx-double_raw_width+2) = val; // (row-2,col+2)
        }
        if (col < 4) {
          RAW_DIRECT_IDX(idx+double_raw_width-2) = val; // (row+2,col-2)
        }
        idx += 2;
      }
    }
  }

  /* Save bitbuff state at end of second loop */
  prev_bitbuf_b = bitbuf;
  prev_vbits_b = vbits;
  prev_offset_b = ftell(ifp);

  /* Row from 2 to 21, accessing pixels from 2 to 21 */
  for (row=2; row < h_plus2; row++) {
    idx = RAW_IDX(row, 3-(row & 1));
    end_idx = RAW_IDX(row, width_plus2);
    for (;idx < end_idx; idx +=2) {
      val = ((RAW_DIRECT_IDX(idx-1) + (RAW_DIRECT_IDX(idx) << 2) +
              RAW_DIRECT_IDX(idx+1)) >> 1) - 0x100;
      RAW_DIRECT_IDX(idx) = LIM(val,0,255);
    }
  }

  /* Row from 0 to 19, moving pixels from row+2 to row */
  for (row=0; row < h; row++) {
    idx = RAW_IDX(row, 0);
    for (col=0; col < width; idx++, col++) {
      /* FIXME do I need the curve there? */
      // RAW(row,col) = curl[RAW(row+2,col+2)]
      RAW_DIRECT_IDX(idx) = curve[RAW_DIRECT_IDX(idx+double_raw_width+2)] >> 2;
    }
  }
}
