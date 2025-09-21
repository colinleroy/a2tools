#include "qtkn_platform.h"
#include "qtk_bithuff.h"

uint8 *raw_ptr1;
extern uint8 *row_idx, *row_idx_plus2;
extern uint8 last;
extern uint16 val;
extern uint8 factor;
extern uint8 buf_0[DATABUF_SIZE];

uint8 *cur_buf_0l, *cur_buf_1l, *cur_buf_2l;
uint8 *cur_buf_0h, *cur_buf_1h, *cur_buf_2h;

void copy_data(void) {
    uint8 x, y;

    raw_ptr1 = row_idx - 1;
    for (y = 4; y; y--) {
      int steps, i, j;
      x = (y & 1);
      if (x) {
        steps = (WIDTH/4)-2;
        x = 0;
      } else {
        steps = (WIDTH/4)-1;
        x = 1;
      }

      for (j = 4; j; j--) {
        for (i = x; i != steps; i+=2) {
          *(raw_ptr1 + (i+1)) = *(raw_ptr1+i);
        }
        raw_ptr1 += (WIDTH/4);
      }
    }

    row_idx += (WIDTH<<2);
    row_idx_plus2 += (WIDTH<<2);
}

void consume_extra(void) {
  uint8 c;
  uint8 col;
  uint8 tree;
  uint8 rep, rep_loop, nreps;
  /* Consume RADC tokens but don't care about them. */
  for (c=1; c != 3; c++) {
    for (tree = 1, col = WIDTH/4; col; ) {
      huff_num = tree*2;
      tree = getctrlhuff();
      if (tree) {
        col--;
        if (tree == 8) {
          getdatahuff8();
          getdatahuff8();
          getdatahuff8();
          getdatahuff8();
        } else {
          huff_num = tree+1;
          getdatahuff();
          getdatahuff();
          getdatahuff();
          getdatahuff();
        }
      } else {
        do {
          if (col > 1) {
            huff_num = 0;
            nreps = getdatahuff() + 1;
          } else {
            nreps = 1;
          }
          if (nreps > 8) {
            rep_loop = 8;
          } else {
            rep_loop = nreps;
          }
          huff_num = 1;
          for (rep=0; rep != rep_loop && col; rep++) {
            col--;
            if (rep & 1) {
              getdatahuff();
            }
          }
        } while (nreps == 9);
      }
    }
  }
}

void init_row(void) {
  uint16 i;
  uint32 tmp32;

  if (last > 17)
    val = (val_from_last[last] * factor) >> 4;
  else
    val = ((val_from_last[last]|(val_hi_from_last[last]<<8)) * factor) >> 4;
  last = factor;
  cur_buf_0l = buf_0;
  cur_buf_0h = buf_0+(DATABUF_SIZE/2);

  if (val == 0x100) {
    /* do nothing */
  } else if (val == 0xFF) {
    for (i = 0; i < USEFUL_DATABUF_SIZE; i++) {
      tmp32 = GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, i);
      tmp32 = tmp32 - (tmp32>>8);
      SET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, i, tmp32);
    }
  } else {
    for (i = 0; i < USEFUL_DATABUF_SIZE; i++) {
      tmp32 = val * GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, i);
      tmp32 >>= 8;
      SET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, i, tmp32);
    }
  }
}
