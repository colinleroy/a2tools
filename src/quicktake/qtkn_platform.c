#include "qtkn_platform.h"
#include "qtk_bithuff.h"
#include "qt-conv.h"

uint8 *raw_ptr1;
extern uint8 *row_idx, *row_idx_plus2;
extern uint8 last;
extern uint16 val;
extern uint8 factor;
extern uint8 buf_0[DATABUF_SIZE];
extern uint8 buf_1[DATABUF_SIZE];
extern uint8 buf_2[DATABUF_SIZE];
extern uint8 div48_l[256];
extern uint8 div48_h[256];
uint8 raw_image[RAW_IMAGE_SIZE];

uint8 *cur_buf_0l, *cur_buf_1l, *cur_buf_2l;
uint8 *cur_buf_0h, *cur_buf_1h, *cur_buf_2h;

void init_shiftl4(void) {
  uint8 c = 0;
  do {
    int8 sc = (int8)c;
    if (sc >= 0) {
      shiftl4p_l[c] = (sc<<4) & 0xFF;
      shiftl4p_h[c] = (sc<<4) >> 8;
      // printf("l4 p[%02X] = %04X\n", c, (uint16)(sc<<4));
    } else {
      shiftl4n_l[c-128] = ((int16)(sc<<4)) & 0xFF;
      shiftl4n_h[c-128] = ((int16)(sc<<4)) >> 8;
      // printf("l4 n[%02X] = %04X [%02X%02X]\n", c-128, (uint16)(sc<<4),
      //        shiftl4n_h[c-128], shiftl4n_l[c-128]);
    }
  } while (++c);
}

#pragma code-name(push, "LC")
void init_buf_0(void) {
  /* init buf_0[*] = 2048 */
  memset(buf_0, 2048 & 0xFF, USEFUL_DATABUF_SIZE);
  memset(buf_0+512, 2048>>8, USEFUL_DATABUF_SIZE);
}

void init_div48(void) {
  uint8 r = 0;
  do {
    /* 48 is the most common multiplier and later divisor.
     * It is worth it to approximate those divisions HARD
     * by rounding the numerator to the nearest 256, in order
     * to have a size-appropriate table. */
    uint16 approx = ((r<<8)|0x80)/48;
    div48_l[r] = approx & 0xFF;
    div48_h[r] = approx >> 8;
    // printf("%d/48 = %d\n", r<<8, div48_l[r]+(div48_h[r]<<8));
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
    // printf("huff_data[%d][%.*b] = %d (r%d)\n", l, r, code, src[i+1], r);

    if (val >> 8 != (val+incr) >> 8) {
      l++;
    }
    val += incr;
  }
}

void init_top(void) {
  init_huff();
  init_div48();
  init_buf_0();
  init_shiftl4();
}

#pragma code-name(pop)

void copy_data(void) {
    row_idx += (WIDTH*2);
    row_idx_plus2 += (WIDTH*2);
}

void consume_extra(void) {
  uint8 c;
  uint8 col;
  uint8 tree;
  uint8 rep, rep_loop, nreps;
  /* Consume RADC tokens but don't care about them. */
  for (c=1; c != 3; c++) {
    for (tree = 1, col = WIDTH/2; col; ) {
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
  uint16 val;

  if (last > 17)
    val = (val_from_last[last] * factor) >> 4;
  else
    val = ((val_from_last[last]|(val_hi_from_last[last]<<8)) * factor) >> 4;
  last = factor;
  cur_buf_0l = buf_0;
  cur_buf_0h = buf_0+(DATABUF_SIZE/2);
  printf("mult %04X\n", val);
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

extern uint8 row;
void decode_row(void) {
  uint8 r, i, y, rep, nreps, rep_loop, tree, tmp8;
  uint16 x;
  int8 tk;
  uint16 tk1, tk2, tk3, tk4;
  uint16 col;


  for (r=0; r != 2; r++) {
    SET_CURBUF_VAL(buf_1, buf_1+(DATABUF_SIZE/2), (WIDTH), (factor<<7));
    SET_CURBUF_VAL(buf_2, buf_2+(DATABUF_SIZE/2), (WIDTH), (factor<<7));

    col = WIDTH;
    cur_buf_0l = buf_0 + col;
    cur_buf_0h = cur_buf_0l+(DATABUF_SIZE/2);
    cur_buf_1l = cur_buf_0l + DATABUF_SIZE;
    cur_buf_1h = cur_buf_1l+(DATABUF_SIZE/2);
    cur_buf_2l = cur_buf_1l + DATABUF_SIZE;
    cur_buf_2h = cur_buf_2l+(DATABUF_SIZE/2);
    tree = 1;

    while(col) {
      huff_num = tree*2;
      tree = (uint8) getctrlhuff();

      if (tree) {
        col-=2;
        cur_buf_0l-=2;
        cur_buf_0h-=2;
        cur_buf_1l-=2;
        cur_buf_1h-=2;
        cur_buf_2l-=2;
        cur_buf_2h-=2;

        if (tree == 8) {
          tmp8 = (uint8) getdatahuff8();
          /* No need for a lookup table here, it's not done a lot
           * and is a 8x8 mult anyway */
          SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1, tmp8 * factor);
          tmp8 = (uint8) getdatahuff8();
          SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0, tmp8 * factor);
          tmp8 = (uint8) getdatahuff8();
          SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1, tmp8 * factor);
          tmp8 = (uint8) getdatahuff8();
          SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0, tmp8 * factor);
        } else {
          huff_num = tree+1;

          //a
          tk1 = ((int8)getdatahuff()) << 4;
          tk2 = ((int8)getdatahuff()) << 4;
          tk3 = ((int8)getdatahuff()) << 4;
          tk4 = ((int8)getdatahuff()) << 4;
          SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1, 
                          (((((GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 2) + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 2)) >> 1)
                            + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 1)) >> 1)
                            + tk1));

          /* Second with col - 1*/
          SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0, 
                          (((((GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1) + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 1)) >> 1)
                            + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 0)) >> 1)
                            + tk2));

          //b
          SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1, 
                          (((((GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 2) + GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 2)) >> 1)
                            + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)) >> 1)
                            + tk3));

          /* Second with col - 1*/
          SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0, 
                          (((((GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1) + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)) >> 1)
                            + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0)) >> 1)
                            + tk4));
        }
      } else {
        do {
          if (col > 2) {
            huff_num = 0;
            nreps = getdatahuff();
            nreps++;
          } else {
            nreps = 1;
          }
          if (nreps > 8) {
            rep_loop = 8;
          } else {
            rep_loop = nreps;
          }
          rep = 0;
          huff_num = 1;
          do_rep_loop:
            col-=2;
            cur_buf_0l-=2;
            cur_buf_0h-=2;
            cur_buf_1l-=2;
            cur_buf_1h-=2;
            cur_buf_2l-=2;
            cur_buf_2h-=2;

            //c
            SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1,
                           (((GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 2)
                           + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 2)) >> 1)
                           + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 1)) >> 1);

            SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0,
                           (((GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)
                           + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 1)) >> 1)
                           + GET_CURBUF_VAL(cur_buf_0l, cur_buf_0h, 0)) >> 1);

            //d
            SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1,
                           (((GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 2)
                           + GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 2)) >> 1)
                           + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)) >> 1);

            SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0,
                           (((GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1)
                           + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)) >> 1)
                           + GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0)) >> 1);

            if (rep & 1) {
              tk = getdatahuff() << 4;
              //e
              SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0, GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 0)+tk);
              SET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1, GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, 1)+tk);
              SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0, GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 0)+tk);
              SET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1, GET_CURBUF_VAL(cur_buf_2l, cur_buf_2h, 1)+tk);
            }
          rep++;
          if (rep == rep_loop)
            goto rep_loop_done;
          if (!col)
            goto rep_loop_done;
          goto do_rep_loop;

          rep_loop_done:
        } while (nreps == 9);
      }
    }

    // Copy to raw buffer
    if (r == 0) {
      raw_ptr1 = row_idx; //FILE_IDX(row, 0);
    } else {
      raw_ptr1 = row_idx_plus2; //FILE_IDX(row + 2, 0);
    }

    cur_buf_1l = buf_1;
    cur_buf_1h = cur_buf_1l + (DATABUF_SIZE/2);

    #if (WIDTH/4) != 80
    #error
    #endif
  
    x = WIDTH-1;
    do {
      uint16 val;
      if (cur_buf_1h[x] & 0x80) {
        val = 0;
      } else {
        if (factor == 48) {
          val = GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, x);
          val = div48_l[val>>8]|(div48_h[val>>8]<<8);
        } else {
          val = GET_CURBUF_VAL(cur_buf_1l, cur_buf_1h, x) / factor;
        }
        if (val > 255)
          val = 255;
      }
      *(raw_ptr1+(x)) = val;
    } while (x--);

    memcpy (buf_0+1, buf_2, (USEFUL_DATABUF_SIZE-1));
    memcpy (buf_0+512+1, buf_2+512, (USEFUL_DATABUF_SIZE-1));
  }
}
