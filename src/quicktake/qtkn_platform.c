#include "qtkn_platform.h"
#include "qtk_bithuff.h"

static uint8 *raw_ptr1;
extern uint8 *row_idx, *row_idx_plus2;

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
