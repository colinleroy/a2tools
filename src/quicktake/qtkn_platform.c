#include "qtkn_platform.h"
#include "qtk_bithuff.h"
#include "qt-conv.h"

static uint8 *dest;
extern uint8 *row_idx;
extern uint8 last;
extern uint16 val;
extern uint8 factor;
uint8 raw_image[RAW_IMAGE_SIZE];

#pragma code-name(push, "LC")
void init_next_line(void) {
  uint16 i;
  for (i = 0; i < DATABUF_SIZE; i++) {
    next_line[i] = 2048;
  }
}

void init_div48(void) {
  uint8 r = 0;
  do {
    /* 48 is the most common multiplier and later divisor.
     * It is worth it to approximate those divisions HARD
     * by rounding the numerator to the nearest 256, in order
     * to have a size-appropriate table. */
#ifdef APPROX_DIVISION
    uint16 approx = (((r<<8)|0x80)*(65536/(48)))>>16;
#else
    uint16 approx = ((r<<8)|0x80)/48;
#endif
    if ((r<<8) & 0x8000) {
      div48_l[r] = 0;
      div48_h[r] = 0xFF;
    } else {
      if (approx >> 8) {
        div48_l[r] = 0xFF;
        div48_h[r] = 0x01;
      } else {
        div48_l[r] = (approx) & 0xFF;
        div48_h[r] = approx >> 8;
      }
    }
  } while (++r);
}

void init_dyndiv(uint8 factor) {
  uint8 r = 0;

  do {
#ifdef APPROX_DIVISION
    uint16 approx = (((r<<8)|0x80)*(65536/(factor)))>>16;
#else
    uint16 approx = ((r<<8)|0x80)/factor;
#endif
    if ((r<<8) & 0x8000) {
      dyndiv_l[r] = 0;
      dyndiv_h[r] = 0xFF;
    } else {
      if (approx >> 8) {
        dyndiv_l[r] = 0xFF;
        dyndiv_h[r] = 0x01;
      } else {
        dyndiv_l[r] = (approx) & 0xFF;
        dyndiv_h[r] = approx >> 8;
      }
    }
  } while (++r);
}

void init_huff(void) {
  static uint8 l, h;
  static uint16 val, src_idx;
  l = 0;
  h = 1;

  for (val = src_idx = 0; l < 18; src_idx += 2) {
    uint8 code = val & 0xFF;
    uint8 numbits, incr;

    numbits = src[src_idx];
    incr = 256 >> numbits;
    code >>= 8-numbits;
    huff_ctrl[h][code] = src[src_idx+1];
    huff_ctrl[l][code] = numbits;
    // printf("huff_ctrl[%d][%.*b] = %d (r%d)\n",
    //        l, numbits, code, src[src_idx+1], numbits);

    if (val >> 8 != (val+incr) >> 8) {
      l += 2;
      h += 2;
    }
    val += incr;
  }

  l = 0;
  h = 1;
  for (; l < 9; src_idx += 2) {
    uint8 code = val & 0xFF;
    uint8 numbits, incr;
    numbits = src[src_idx];
    incr = 256 >> numbits;

    code >>= 8-numbits;
    huff_data[l][code+128] = src[src_idx+1];
    huff_data[l][code] = numbits;
    // printf("huff_data[%d][%.*b] = %d (%d bits)\n", l, numbits, code, src[src_idx+1], numbits);

    if (val >> 8 != (val+incr) >> 8) {
      l++;
    }
    val += incr;
  }
}

void init_top(void) {
  init_huff();
  init_div48();
  init_next_line();
}

#pragma code-name(pop)

void consume_extra(void) {
  uint8 c;
  uint8 col;
  uint8 tree;
  uint8 rep, rep_loop, nreps;
  /* Consume RADC tokens but don't care about them. */
  for (c=1; c != 3; c++) {
    for (tree = 1, col = WIDTH/2; col; ) {
      tree = getctrlhuff(tree*2);
      if (tree) {
        col--;
        if (tree == 8) {
          getdatahuff8();
          getdatahuff8();
          getdatahuff8();
          getdatahuff8();
        } else {
          getdatahuff(tree+1);
          getdatahuff(tree+1);
          getdatahuff(tree+1);
          getdatahuff(tree+1);
        }
      } else {
        do {
          if (col > 1) {
            nreps = getdatahuff(0) + 1;
          } else {
            nreps = 1;
          }
          if (nreps > 8) {
            rep_loop = 8;
          } else {
            rep_loop = nreps;
          }

          col -= rep_loop;
          rep_loop /= 2;
          while (rep_loop--) {
            getdatahuff(1);
          }

        } while (nreps == 9);
      }
    }
  }
}

uint8 lastdyn = 0;
void init_row(void) {
  uint16 i;
  uint16 tmp32;
  uint16 val;

  if (last > 17)
    val = (val_from_last[last] * factor) >> 4;
  else
    val = ((val_from_last[last]|(val_hi_from_last[last]<<8)) * factor) >> 4;
  if (factor != 48) {
    if (lastdyn != factor) {
      lastdyn = factor;
      init_dyndiv(factor);
    }
  }
  last = factor;

  if (val == 0x100) {
    /* do nothing */
  } else if (val == 0xFF) {
    for (i = 0; i < DATABUF_SIZE-1; i++) {
      tmp32 = next_line[i];
      next_line[i] = tmp32 - (tmp32>>8);
    }
  } else {
    for (i = 0; i < DATABUF_SIZE-1; i++) {
      tmp32 = (val * next_line[i]) >> 8;
      next_line[i] = tmp32;
    }
  }
}

#define SET_OUTPUT(offset, value) do {                                          \
  dest[col+offset] = (factor == 48) ? div48_l[((uint16)value)>>8] : dyndiv_l[((uint16)value)>>8]; \
} while(0)

void decode_row(void) {
  uint8 r, i, y, rep, nreps, rep_loop, tree, tmp8;
  int16 tk;
  int16 tk1, tk2, tk3, tk4;
  int16 col;
  int16 val1, val0;

  for (r=0; r != 2; r++) {
    val0 = ((int16)factor)<<7;
    next_line[WIDTH+1] = factor << 7;

    row_idx += WIDTH;
    dest = row_idx;

    col = WIDTH;
    tree = 1;

    while(col) {
      tree = (uint8) getctrlhuff(tree*2);

      if (tree) {
        col-=2;

        if (tree == 8) {
          tmp8 = (uint8) getdatahuff8();
          dest[col+1] = tmp8;
          val1 = tmp8 * factor;

          tmp8 = (uint8) getdatahuff8();
          dest[col+0] = tmp8;
          val0 = tmp8 * factor;

          tmp8 = (uint8) getdatahuff8();
          next_line[col+2] = tmp8 * factor;
          tmp8 = (uint8) getdatahuff8();
          next_line[col+1] = tmp8 * factor;
        } else {
          tk1 = ((int8)getdatahuff(tree+1)) << 4;
          tk2 = ((int8)getdatahuff(tree+1)) << 4;
          tk3 = ((int8)getdatahuff(tree+1)) << 4;
          tk4 = ((int8)getdatahuff(tree+1)) << 4;

          val1 = ((((val0 + next_line[col+2]) >> 1)
                  + next_line[col+1]) >> 1)
                  + tk1;
          SET_OUTPUT(1, val1);

          next_line[col+2] = ((((val0 + next_line[col+3]) >> 1)
                              + val1) >> 1)
                              + tk3;

          val0 = ((((val1 + next_line[col+1]) >> 1)
                  + next_line[col+0]) >> 1)
                  + tk2;
          SET_OUTPUT(0, val0);

          next_line[col+1] = ((((val1 + next_line[col+2]) >> 1)
                              + val0) >> 1)
                              + tk4;
        }
      } else {
        do {
          nreps = (col > 2) ? getdatahuff(0) + 1 : 1;
          rep_loop = nreps;
          if (rep_loop > 8) {
            rep_loop = 8;
          }

          for (rep = 0; rep < rep_loop; rep++) {
            col-=2;

            val1 = ((((val0 + next_line[col+2]) >> 1)
                    + next_line[col+1]) >> 1);
            SET_OUTPUT(1, val1);

            next_line[col+2] = ((((val0 + next_line[col+3]) >> 1)
                                + val1) >> 1);

            val0 = ((((val1 + next_line[col+1]) >> 1)
                    + next_line[col+0]) >> 1);
            SET_OUTPUT(0, val0);

            next_line[col+1] = ((((val1 + next_line[col+2]) >> 1)
                                + val0) >> 1);

            if (rep & 1) {
              tk = ((int8)getdatahuff(1)) << 4;
              //e
              val1 += tk;
              SET_OUTPUT(1, val1);

              val0 += tk;
              SET_OUTPUT(0, val0);

              next_line[col+2] += tk;
              next_line[col+1] += tk;
            }
          }
        } while (nreps == 9);
      }
    }
  }
}
